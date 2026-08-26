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
  perSystem = {
    config,
    pkgs,
    ...
  }: let
    src = pkgs.lib.fileset.toSource {
      root = ../.;
      fileset = pkgs.lib.fileset.unions [
        ../CMakeLists.txt
        ../LICENSE
        ../src
      ];
    };
    mkEunha = cmakeBuildType:
      pkgs.clangStdenv.mkDerivation {
        pname = "eunha";
        version = "0.1.0";

        inherit cmakeBuildType src;

        nativeBuildInputs = with pkgs; [
          cmake
          ninja
        ];

        cmakeFlags = ["-DBUILD_TESTING=OFF"];
        doCheck = false;

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
      default = mkEunha "Release";
      debug = mkEunha "Debug";
      check = pkgs.writeShellApplication {
        name = "eunha-check";
        runtimeInputs = with pkgs; [
          cmake
          clang
          clang-tools
          findutils
          ninja
        ];
        text = builtins.readFile ../scripts/check.sh;
      };
    };

    apps.default = {
      type = "app";
      program = config.packages.default;
      meta.description = "Run eunha";
    };
  };
}
