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
{inputs, ...}: let
  paths = [
    ".github/dependabot.yml"
    "Makefile"
    "**/*.nix"
    "**/*.c"
    "**/*.h"
    "**/Makefile"
    "bear.yaml"
    "flake.lock"
    ".github/workflows/checks.yml"
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
