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
{inputs, ...}: {
  perSystem = {system, ...}: let
    pkgs = import inputs.nixpkgs {
      inherit system;
      overlays = [(import inputs.rust-overlay)];
    };

    rustToolchain = pkgs.rust-bin.stable.latest.default.override {
      extensions = [
        "clippy"
        "rust-src"
        "rustfmt"
        "rust-analyzer"
      ];
    };
    craneLib = (inputs.crane.mkLib pkgs).overrideToolchain rustToolchain;
    eunhaCraneArgs = {
      pname = "eunha";
      version = "0.1.0";
      src = craneLib.cleanCargoSource ../.;
      strictDeps = true;
    };
    eunhaCargoArtifacts = craneLib.buildDepsOnly eunhaCraneArgs;
  in {
    _module.args = {
      inherit
        craneLib
        eunhaCargoArtifacts
        eunhaCraneArgs
        pkgs
        rustToolchain
        ;
    };
  };
}
