#!/usr/bin/env bash
# Instala dependencias para o ambiente de desenvolvimento Debian do Jay Frontend

set -e

echo "Instalando dependencias de compilacao C++ e bibliotecas..."
sudo apt update
sudo apt install -y build-essential g++ make cmake

echo "Instalando dependencias graficas (Raylib)..."
sudo apt install -y libraylib-dev

echo "Instalando nlohmann-json (JSON C++ header-only)..."
sudo apt install -y nlohmann-json3-dev

echo "Instalacao concluida com sucesso!"
