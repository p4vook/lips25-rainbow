{
  description = "A very basic flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs = { self, nixpkgs }:
  let 
  system = "aarch64-darwin";
  pkgs = nixpkgs.legacyPackages."${system}";
  llvm = pkgs.llvmPackages_21;
  in {
    devShells."${system}".default = pkgs.mkShell.override { stdenv = pkgs.gcc14Stdenv; } {
      packages = [
        pkgs.git
        pkgs.gdb
        pkgs.hyperfine
        pkgs.gnumake
        llvm.clang-tools
      ];
    };
  };
}
