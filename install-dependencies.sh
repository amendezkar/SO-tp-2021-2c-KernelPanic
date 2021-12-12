# Instalamos las commons

cd ~

git clone https://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@github.com/sisoputnfrba/so-commons-library.git
cd so-commons-library
make install
# Instalamos las pruebas

cd ~/tp-2021-2c-KernelPanic
make clean all
sudo cp matelib/bin/libmatelib.so /usr/local/lib/libmatelib.so
sudo cp static/bin/libstatic.a /usr/local/lib/libstatic.a

cd ~
git clone https://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@github.com/sisoputnfrba/carpinchos-pruebas.git
cd carpinchos-pruebas
make install


#aca hay q meterse al makefile y agregar l-static