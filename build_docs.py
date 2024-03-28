import os

if not os.path.exists("build/docs/doxygen-awesome-css"):
  print("[INFO] Cloning CSS for docs...")
  os.system("git clone --recursive https://github.com/jothepro/doxygen-awesome-css.git build/docs/doxygen-awesome-css")

print("[INFO] doxygen-awesome-css is cloned")

print("[INFO] Building Liger-Engine docs...")
os.system("doxygen Docs/Doxyfile")