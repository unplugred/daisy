{
	inputs = {
		nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
		flake-utils.url = "github:numtide/flake-utils";
	};

	outputs = { self, nixpkgs, flake-utils, ... }:
	flake-utils.lib.eachDefaultSystem (system:
	let
		pkgs = nixpkgs.legacyPackages.${system};
		pkgs_arm_gcc = import (builtins.fetchGit {
			url = "https://github.com/nixos/nixpkgs/";
			rev = "eaff114326e814083bcb208caa97eab1fbedd79b";
		}) {
			inherit system;
		};
	in {
		devShells.default = pkgs.mkShell {
			buildInputs = [ # packages
				pkgs.git
				(pkgs.python3.withPackages (ps: with ps; [ # python packages
				]))
				pkgs_arm_gcc.gcc-arm-embedded-10
				#pkgs.gnumake
				pkgs.dfu-util
				#pkgs.openocd
				pkgs.platformio
			];

			# environment variables
			PROJECT_NAME = "daisy";
		};
	});
}
