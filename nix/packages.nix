{
  perSystem = {
    config,
    pkgs,
    ...
  }: let
    src = pkgs.lib.cleanSourceWith {
      src = ../.;
      filter = path: type: let
        rel = pkgs.lib.removePrefix "${toString ../.}/" (toString path);
      in
        !(rel == "build" || pkgs.lib.hasPrefix "build/" rel || rel == "result");
    };
  in {
    packages.default = pkgs.clangStdenv.mkDerivation {
      pname = "eunha";
      version = "0.1.0";

      inherit src;

      nativeBuildInputs = with pkgs; [
        cmake
        ninja
      ];

      meta.mainProgram = "eunha";
    };

    checks.package = config.packages.default;

    apps.default = {
      type = "app";
      program = config.packages.default;
      meta.description = "Run eunha";
    };
  };
}
