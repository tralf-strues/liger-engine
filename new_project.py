import argparse, os, shutil

# Parse arguments
parser = argparse.ArgumentParser(description='Create new Liger Project')

parser.add_argument('--name', help='Project name', type=str)
parser.add_argument('--path', help='Absolute path where to create the project')
parser.add_argument('--liger_path', help='Absolute path to the Liger-Engine directory')
parser.add_argument('--template', nargs='?', help='Name of the project template (currently only Empty is supported)', default="Empty")

args = parser.parse_args()

project_name = args.name
project_path = args.path
engine_path = args.liger_path
template_name = args.template

print(f"[INFO] project_name = {project_name}")
print(f"[INFO] project_path = '{project_path}'")
print(f"[INFO] engine_path = '{engine_path}'")
print(f"[INFO] template_name = {template_name}")

if os.path.exists(project_path):
  print(f"[ERROR] Project path already exists '{project_path}'")
  exit(1)

if not os.path.exists(engine_path):
  print(f"[ERROR] Invalid liger_path '{engine_path}'")
  exit(1)

template_path = engine_path + "/ProjectTemplates"
if template_name == "Empty":
  template_path += "/Empty"
else:
  print("[ERROR] Invalid project template!")
  exit(1)

print(f"[INFO] template_path = '{template_path}'")
assert(os.path.exists(template_path))

# Copy template
shutil.copytree(src=template_path, dst=project_path)

# Copy assets
engine_assets_path = engine_path + "/Assets"
project_assets_path = project_path + "/Assets"
shutil.copytree(src=engine_assets_path, dst=project_assets_path)

# Copy .gitignore
shutil.copy(src=engine_path + "/.gitignore", dst=project_path + "/.gitignore")

# Set project name in CMake
project_cmake_filepath = project_path + "/CMakeLists.txt"
with open(project_cmake_filepath, "r+") as cmake_file:
  cmake_contents = cmake_file.read()
  cmake_file.seek(0)

  cmake_contents = cmake_contents.replace("Empty-Project", project_name)
  cmake_file.write(cmake_contents)

# Set engine path to build script
build_script_filepath = project_path + "/build_unix.sh"
with open(build_script_filepath, "r+") as build_script_file:
  build_script = build_script_file.read()
  build_script_file.seek(0)

  build_script = build_script.replace("-DLIGER_ENGINE_PATH=\"\"", f"-DLIGER_ENGINE_PATH=\"{engine_path}\"")
  build_script = build_script.replace("-DLiger-Engine_DIR=\"\"", f"-DLiger-Engine_DIR=\"{engine_path}/${{build_dir_name}}/Engine\"")
  build_script_file.write(build_script)