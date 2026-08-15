{inputs, ...}: let
  paths = [
    ".clang-format"
    ".clangd"
    "**/*.nix"
    "CMakeLists.txt"
    "CMakePresets.json"
    "flake.lock"
    ".github/workflows/checks.yml"
    "src/**"
  ];
in {
  imports = [
    inputs.actions-nix.flakeModules.default
  ];

  flake.actions-nix = {
    pre-commit.enable = true;
    workflows = {
      ".github/workflows/checks.yml" = {
        name = "checks";

        on = {
          pull_request = {
            inherit paths;
          };
          push = {
            branches = ["main"];
            inherit paths;
          };
        };

        jobs.flake = {
          runs-on = "ubuntu-latest";
          steps = [
            {uses = "actions/checkout@v7";}
            {uses = "DeterminateSystems/nix-installer-action@main";}
            {uses = "DeterminateSystems/magic-nix-cache-action@main";}
            {
              name = "Run nix flake check";
              run = "nix flake check";
            }
            {
              name = "Build package";
              run = "nix build";
            }
          ];
        };
      };
    };
  };
}
