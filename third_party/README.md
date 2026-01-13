# Third-Party Dependencies

This directory contains third-party libraries integrated as Git submodules.

## Dependencies

- **SPIRV-Cross**: SPIR-V to GLSL/HLSL/MSL converter
  - Repository: https://github.com/KhronosGroup/SPIRV-Cross
  - Version: sdk-1.3.261.1

- **glslang**: GLSL/HLSL to SPIR-V compiler
  - Repository: https://github.com/KhronosGroup/glslang
  - Version: 13.0.0

- **GLFW**: Window and input management
  - Repository: https://github.com/glfw/glfw
  - Version: 3.3.8

- **GLM**: Mathematics library
  - Repository: https://github.com/g-truc/glm
  - Version: 0.9.9.8

## Setup

When cloning this repository for the first time, initialize submodules:

```bash
git submodule update --init --recursive
```

## Updating Dependencies

To update a submodule to the latest version:

```bash
cd third_party/spirv-cross
git checkout <new-version-tag>
cd ../..
git add third_party/spirv-cross
git commit -m "Update SPIRV-Cross to <version>"
```

## Modifying Dependencies

You can directly modify the source code in these directories. The changes will be tracked in your main repository.

**Note**: If you modify submodule code, you may want to commit those changes separately or create a fork of the original repository.
