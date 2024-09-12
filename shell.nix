let
  nixpkgs = fetchTarball "https://github.com/NixOS/nixpkgs/tarball/24.05";
  rust-overlay = fetchTarball "https://github.com/oxalica/rust-overlay/tarball/e5d3f9c2f24d852cddc79716daf0f65ce8468b28";
  pkgs = import nixpkgs { overlays = [ (import rust-overlay) ]; };

  llvmPackages = pkgs.llvmPackages_14;
  MacOSX-SDK = pkgs.darwin.apple_sdk_11_0.MacOSX-SDK;
  Security = pkgs.darwin.apple_sdk_11_0.frameworks.Security;
  latest-odin = with pkgs; stdenv.mkDerivation rec {
    pname = "odin";
    version = "dev-2024-09";

    src = fetchFromGitHub {
      owner = "odin-lang";
      repo = "Odin";
      rev = version;
      hash = "sha256-rbKaGj4jwR+SySt+XJ7K9rtpQsL60IKJ55/1uNkVE1U=";
    };

    nativeBuildInputs = [
      makeBinaryWrapper
      which
    ];

    buildInputs = lib.optionals stdenv.isDarwin [
      libiconv
      Security
    ];

    LLVM_CONFIG = "${llvmPackages.llvm.dev}/bin/llvm-config";

    postPatch = lib.optionalString true ''
      echo ${MacOSX-SDK}
      substituteInPlace src/linker.cpp --replace-fail '/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk' ${MacOSX-SDK}
    '' + ''
      substituteInPlace build_odin.sh \
        --replace-fail '-framework System' '-lSystem'
      patchShebangs build_odin.sh
    '';

    dontConfigure = true;

    buildFlags = [
      "release"
      ];

      installPhase = ''
        runHook preInstall

        mkdir -p $out/bin
        cp odin $out/bin/odin

        mkdir -p $out/share
        cp -r base $out/share/base
        cp -r core $out/share/core
        cp -r vendor $out/share/vendor

        wrapProgram $out/bin/odin \
        --prefix PATH : ${lib.makeBinPath (with llvmPackages; [
          bintools
          llvm
          clang
          lld
        ])} \
        --set-default ODIN_ROOT $out/share

        runHook postInstall
      '';
    };
in
  pkgs.mkShell {
    packages = with pkgs; [
      latest-odin
      lld
      wasm3

      cmake
      rust-bin.stable.latest.default

      nodejs-18_x
    ] ++ lib.optionals stdenv.isDarwin [
      darwin.apple_sdk.frameworks.CoreData
      darwin.apple_sdk.frameworks.Kernel
      darwin.apple_sdk.frameworks.CoreVideo
      darwin.apple_sdk.frameworks.GLUT
      darwin.apple_sdk.frameworks.IOKit
      darwin.apple_sdk.frameworks.OpenGL
      darwin.apple_sdk.frameworks.Cocoa
    ];
  }
