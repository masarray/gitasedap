# Third-party dependencies

## iPlug2

- Project: iPlug2
- Purpose: plugin-format wrapper and IGraphics UI foundation
- Upstream: https://github.com/iPlug2/iPlug2
- Pinned commit: `d54f69050f517e43b941d88c2a170f0a840b9ee4`
- Upstream licence: liberal zlib-like licence; see the upstream `LICENSE.txt`
- Integration: fetched at CMake configure time unless a local checkout is explicitly supplied

## Steinberg VST3 SDK

- Project: VST3 SDK
- Purpose: VST3 public interfaces and SDK implementation required by the initial Windows plug-in target
- Upstream: https://github.com/steinbergmedia/vst3sdk
- Pinned commit: `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` (VST3 SDK 3.8.1)
- Integration: fetched into iPlug2's expected public-SDK location during dependency preparation
- Submodules fetched: `base`, `cmake`, `pluginterfaces`, `public.sdk`

Dependency revisions are intentionally pinned. Updating either revision requires a dedicated dependency change with build and host regression verification.
