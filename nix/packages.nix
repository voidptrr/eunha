# MIT License
#
# Copyright (c) 2026 Tommaso Bruno
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
{
  perSystem = {pkgs, ...}: let
    format = pkgs.writeShellApplication {
      name = "format";
      runtimeInputs = with pkgs; [findutils clang-tools alejandra];
      text = builtins.readFile ../scripts/format.sh;
    };
    build = pkgs.writeShellApplication {
      name = "build";
      runtimeInputs = [pkgs.clang];
      text = builtins.readFile ../scripts/build.sh;
    };
    pre-checks = pkgs.writeShellApplication {
      name = "pre-checks";
      runtimeInputs = with pkgs; [
        alejandra
        bear
        clang
        clang-tools
        findutils
      ];
      text = builtins.readFile ../scripts/pre-checks.sh;
    };
    src = pkgs.lib.fileset.toSource {
      root = ../.;
      fileset = pkgs.lib.fileset.unions [
        ../LICENSE
        ../src
      ];
    };
    mkEunha = mode:
      pkgs.clangStdenv.mkDerivation {
        pname = "eunha";
        version = "0.1.0";

        inherit src;

        nativeBuildInputs = [build];
        dontConfigure = true;

        buildPhase = ''
            runHook preBuild
            build ${mode}
          runHook postBuild
        '';

        installPhase = ''
          runHook preInstall
          install -Dm755 build/${mode}/eunha "$out/bin/eunha"
          runHook postInstall
        '';

        meta = with pkgs.lib; {
          homepage = "https://github.com/voidptrr/eunha";
          license = licenses.mit;
          mainProgram = "eunha";
          maintainers = [
            {
              name = "Tommaso Bruno";
              github = "voidptrr";
            }
          ];
          platforms = platforms.linux;
        };
      };
  in {
    packages = {
      inherit build pre-checks format;
      default = mkEunha "release";
      debug = mkEunha "debug";
    };
  };
}
