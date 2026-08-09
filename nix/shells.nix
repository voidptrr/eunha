{
  perSystem = {
    config,
    pkgs,
    ...
  }: {
    devShells.default = pkgs.mkShellNoCC {
      shellHook = config.pre-commit.installationScript;

      packages = [];
    };
  };
}
