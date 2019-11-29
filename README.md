# frak
Frak is a cli fractal generator written in C. This is a toy project for myself to explore what I call "creative programming". The roadmap is fuzzy; some directions are in this project's issues page, but really anything reasonable is welcome.

## CI
I setup TravisCI! The testing infrastructure is built from scratch in C. The interface is somewhat similar to gtest.

[![Build Status](https://travis-ci.com/danzimm/frak.svg?branch=master)](https://travis-ci.com/danzimm/frak)

## Contributions
Contributions are welcome! If there's something cool that you want to work on with an issue open then feel free to create a PR! Please run clang-format before submitting. You can run tests locally via the target `t`.

## Example
![Example](/mand.png)
This was created with the command `./frak --palette custom --width 10000 --height 10000 --design mand mand.tiff --color 0,0,0,0 --color 50,0,97,255 --color 254,255,255,255 --color 255,0,0,0 && magick mand.tiff mand.png`
