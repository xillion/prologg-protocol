import re
import os
import sys
from jinja2 import Environment, FileSystemLoader, select_autoescape

# Directory containing the .j2 template files
TEMPLATES_DIR = os.path.join(os.path.dirname(__file__), "templates")


def generate_dispatch(pb_h_path, template_dir, output_dir):
    """
    Parses Request tags from prologg.pb.h and generates a C++ dispatch table
    using Jinja2 templates.
    """
    out_h = os.path.join(output_dir, "prologg_dispatch.h")
    out_cpp = os.path.join(output_dir, "prologg_dispatch.cpp")

    os.makedirs(output_dir, exist_ok=True)

    # Regex to find tags like: #define Request_get_time_tag 2
    tag_pattern = re.compile(r'#define\s+(Request_([a-zA-Z0-9_]+)_tag)\s+(\d+)')

    tags = []
    try:
        with open(pb_h_path, 'r') as f:
            content = f.read()
            tags = tag_pattern.findall(content)
    except FileNotFoundError:
        print(f"Error: Protocol header not found at {pb_h_path}")
        sys.exit(1)

    # Sort tags by their numeric ID so the array is built in order
    tags.sort(key=lambda x: int(x[2]))

    # Set up Jinja2 environment pointing at the templates folder
    env = Environment(
        loader=FileSystemLoader(TEMPLATES_DIR),
        autoescape=select_autoescape([]),   # No HTML escaping for C++ output
        keep_trailing_newline=True,
        trim_blocks=True,                   # Strip newline after block tags
        lstrip_blocks=True,                 # Strip leading whitespace before block tags
    )

    context = {"tags": tags}

    # --- Render and write header ---
    h_template = env.get_template("/prologg_dispatch.h.j2")
    with open(out_h, 'w') as f:
        f.write(h_template.render(context))

    # --- Render and write source ---
    cpp_template = env.get_template("/prologg_dispatch.cpp.j2")
    with open(out_cpp, 'w') as f:
        f.write(cpp_template.render(context))

    print(f"Generated: {out_h}")
    print(f"Generated: {out_cpp}")


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python3 generate_dispatch.py <path_to_pb_h> <template_directory> <output_directory>")
        sys.exit(1)

    pb_h_input = sys.argv[1]
    template_dir = sys.argv[2]
    out_directory = sys.argv[3]

    generate_dispatch(pb_h_input, template_dir, out_directory)
