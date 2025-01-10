# Adafruit GFX Library
# https://github.com/adafruit/Adafruit-GFX-Library
# BSD License

cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "src/TMC5160.cpp" "src/TMC5160_SPI.cpp" 
                       INCLUDE_DIRS "src"
                       REQUIRES arduino ArxTypeTraits)

#project(Adafruit-GFX-Library)
