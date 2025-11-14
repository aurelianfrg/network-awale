{
  description = "A C development shell";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in 
    {
    devShells.${system}.default = pkgs.mkShell {
      packages = [
        pkgs.libgcc
        pkgs.gnumake
        pkgs.gdb
        pkgs.valgrind-light
        # Add other C development packages as needed
      ];
      shellHook = ''
        echo "Entering C development shell"
        # Sets the prompt text
        export PS1="\[\033[1;4;38;5;63m\]C-DEVSHELL\[\033[0m\] \[\033[3;38;5;105m\]\w\$\[\033[0m\] "
      '';
    };
  };
}
