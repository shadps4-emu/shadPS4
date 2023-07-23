# How to build shadps4 in windows 

## Download VStudio Community 2022 17.7

Atm it is on preview stage but there seems to be issues using clang + cmake + ninja in the current vstudio (17.6.5 )
[vstudio 17.6.5 bug](https://developercommunity.visualstudio.com/t/cmake-generates-bad-dependencies-for-rc/10398924?q=cmake%20dependencies)

So here is the link for vstudio 2020 17.7 preview (atm it is in preview 4)


[vstudio 17.7 preview](https://learn.microsoft.com/en-us/visualstudio/releases/2022/release-notes-preview)

## Requirements

Install the following

- Desktop development with c++

### From Individual components tab install

- C++ Clang Compiler for Windows (16.0.5)
- MSBuild support for LLVM (clang-cl) toolset

- ## Compiling

- Open vstudio and select the clang debug or clang release . It should compile just fine
