echo "Cloning CSS for docs..."
git clone --recursive https://github.com/jothepro/doxygen-awesome-css.git build/docs/doxygen-awesome-css

echo "Building liger docs..."
doxygen docs/Doxyfile