# prologg-protocol

Единый источник истины для протокола обмена между прошивками PT-Logger
(`pt-logger`, `pt-logger-test`) и ПК-приложением (`spcviewer`).

Ранее `prologg.proto` существовал в трёх независимых копиях (по одной в
каждом репозитории) плюс отдельно сгенерированная Python-копия для
тестового харнесса — они расходились без предупреждения. Этот репозиторий
устраняет расхождение: подключается как git submodule во все
проекты-потребители, схема редактируется только здесь.

## Состав

- `prologg.proto` — схема сообщений протокола.
- `prologg.options` — nanopb-опции (ограничения размеров bytes/string полей).
- `generate_dispatch.py` + `templates/*.j2` — генератор C++ диспетчера
  запросов (`prologg_dispatch.{h,cpp}`) на основе тегов, вычитанных из
  сгенерированного `prologg.pb.h`.
- `CMakeLists.txt` — сборочная цель `prologg`, используется как есть через
  `add_subdirectory(libs/prologg)` в прошивках без изменений.
- `nanopb/` — submodule на upstream [nanopb](https://github.com/nanopb/nanopb),
  закреплён на теге `nanopb-0.4.9.1` (версия, ранее вендоренная копией в
  обоих прошивочных репозиториях).

## Использование в прошивке (pt-logger / pt-logger-test)

Подключается как submodule по пути `libs/prologg`:

```
git submodule add git@github.com:xillion/prologg-protocol.git libs/prologg
git submodule update --init --recursive
```

`CMakeLists.txt` прошивки продолжает вызывать
`add_subdirectory(libs/prologg)` без изменений — весь путь генерации
(`nanopb_generate_cpp` → `generate_dispatch.py` → `prologg_dispatch.*`)
идентичен для обеих прошивок.

## Использование в ПК-приложении (spcviewer)

Подключается как submodule (путь и название — на усмотрение приложения,
рекомендуется `protocol/prologg`). Регенерация Dart-биндингов выполняется
скриптом, который читает `.proto` **только** из этого submodule, с
зафиксированными версиями `protoc`/`protoc-gen-dart` (см. README
`spcviewer`).

## Изменение протокола

1. Редактировать `.proto`/`.options` только в этом репозитории.
2. Закоммитить и запушить сюда.
3. В каждом репозитории-потребителе обновить submodule
   (`git submodule update --remote libs/prologg`) и закоммитить новый
   pinned-commit submodule'я.
4. CI каждого потребителя проверяет, что сгенерированный код совпадает с
   закоммиченным (защита от забытой регенерации).
