{
  perSystem = {
    config,
    pkgs,
    ...
  }: {
    devShells.default = (pkgs.mkShell.override {stdenv = pkgs.clangStdenv;}) {
      shellHook = config.pre-commit.installationScript;
      packages = with pkgs; [
        clang-tools
        cmake
        ninja
      ];
    };
  };
}
