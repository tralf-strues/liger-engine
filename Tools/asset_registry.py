import argparse, os, yaml, re
from scanf import scanf
from struct import unpack
from os import urandom

class AssetEntry:
  asset_file = ""
  asset_id   = 0x0

  def __init__(self, asset_file = "", asset_id = 0x0):
    self.asset_file = asset_file
    self.asset_id   = asset_id

manage_running = False
file_modified = False
asset_folder = ""

def ReadAssetRegistry(filepath):
  with open(filepath, "r+") as registry_file:
    yaml_root = yaml.safe_load(registry_file)

  asset_registry = []
  if yaml_root:
    for yaml_entry in yaml_root:
      asset_entry            = AssetEntry()
      asset_entry.asset_file = yaml_entry["file"]
      asset_entry.asset_id   = yaml_entry["id"]

      asset_registry.append(asset_entry)

  return asset_registry

def GetAssetId(asset_registry, asset_file):
  for asset_entry in asset_registry:
    if asset_entry.asset_file == asset_file:
      return asset_entry.asset_id

  return None

def AddAsset(asset_registry, asset_file):
  global file_modified

  asset_id = GetAssetId(asset_registry, asset_file)
  if asset_id != None:
    print(f"Asset '{asset_file}' is already registered, id = 0x{asset_id:X}")
    return

  abs_filepath = os.path.join(asset_folder, asset_file)
  if not os.path.exists(abs_filepath):
    print(f"No such asset file '{abs_filepath}'")
    return True
  elif not os.path.isfile(abs_filepath):
    print(f"Path '{abs_filepath}' is not a file")
    return True

  asset_id = unpack("!Q", urandom(8))[0]
  asset_registry.append(AssetEntry(asset_file, asset_id))
  file_modified = True

  print(f"Added asset '{asset_file}' with id = 0x{asset_id:X}")

def AddAssets(asset_registry, asset_file_regex):
  any_match = False
  for root, _, files in os.walk(asset_folder):
    for file in files:
      file = os.path.relpath(os.path.join(root, file), asset_folder)
      file = file.replace(os.sep, '/')
      if asset_file_regex.match(file):
        any_match = True
        AddAsset(asset_registry, file)

  if not any_match:
    print(f"No files matched {asset_file_regex}")

def ProcessExit(cmd, asset_registry):
  if cmd != "exit":
    return False

  global manage_running
  manage_running = False

  return True

def ProcessLS(cmd, asset_registry):
  if cmd != "ls":
    return False

  for entry in asset_registry:
    print(f"[file: '{entry.asset_file}', id: 0x{entry.asset_id:X}]")

  return True

def ProcessAdd(cmd, asset_registry):
  asset_file_regex = scanf("add %s", cmd)
  if not asset_file_regex:
    return False

  asset_file_regex = re.compile(asset_file_regex[0])
  AddAssets(asset_registry, asset_file_regex)

  return True

def ProcessCommand(cmd, asset_registry):
  functions = [ProcessExit, ProcessLS, ProcessAdd]

  for function in functions:
    if function(cmd, asset_registry):
      return True

  return False

def ManageRegistry(asset_registry):
  while manage_running:
    print("- ", end="")
    cmd = input()
    if not ProcessCommand(cmd, asset_registry):
      print(f"Invalid command '{cmd}'")

def SaveRegistry(filepath, asset_registry):
  with open(filepath, "w") as registry_file:
    for asset_entry in asset_registry:
      registry_file.write(f"- file: {asset_entry.asset_file}\n")
      registry_file.write(f"  id: 0x{asset_entry.asset_id:X}\n")

# Parse arguments
parser = argparse.ArgumentParser(description='Offline manage asset registry')
parser.add_argument('--file', help='Path to the valid .lregistry file', required=True)
args = parser.parse_args()

if not os.path.exists(args.file):
  print(f"[ERROR] No such registry file '{args.file}'")
  exit(1)

asset_folder = os.path.normpath(os.path.join(args.file, os.pardir))
assert(os.path.exists(asset_folder))

print(f"Asset folder: '{asset_folder}'")

# Read registry
asset_registry = ReadAssetRegistry(args.file)

# Work with registry
manage_running = True
ManageRegistry(asset_registry)

# Save
if file_modified:
  print("File modified, save changes? [Y/n]")

  should_save = None
  while should_save == None:
    print("- ", end="")

    answer = input()
    if answer == "y" or answer == "":
      should_save = True
    elif answer == "n":
      should_save = False

  if should_save:
    SaveRegistry(args.file, asset_registry)