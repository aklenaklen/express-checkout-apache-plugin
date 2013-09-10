#!/bin/bash

CURR_DIR=`pwd`
rm -rf psc-commons-master master.zip*
rm -rf out.log err.log

echo "-----------------------------------------"
echo "Installing apache2-threaded-dev and libcurl4-openssl-dev..."
sudo apt-get install apache2-threaded-dev libcurl4-openssl-dev > out.log 2> err.log
if [ `ls -al $CURR_DIR | grep err.log | awk '{print $5}'` != '0' ]
then
	cat $CURR_DIR/err.log
	exit
fi
 
echo "-----------------------------------------"
echo "Downloading latest mod_paypal_ec from github.paypal.com..."
wget -a out.log https://github.paypal.com/ProfessionalServiceEngineering/psc-commons/archive/master.zip
if [ ! -f master.zip ]
then
	echo "Could not download http://github.paypal.com/ProfessionalServiceEngineering/psc-commons/archive/master.zip"
	exit
fi

echo "Unzipping tar ball..."
unzip master.zip >> out.log

if [ ! -d psc-commons-master/mod_paypal_ec/src ]
then
	echo "Could not find mod_paypal_ec codebase in the tar ball"
	exit
fi

echo "-----------------------------------------"
echo "Compiling the module..."
cd psc-commons-master/mod_paypal_ec/src
apxs2 -c mod_paypal_ec.c 

APACHE_MODULES_DIR=/usr/lib/apache2/modules

while [ ! -d $APACHE_MODULES_DIR ]
do
	echo "Could not find $APACHE_MODULES_DIR"
	echo "Please provide the correct location: "
	read APACHE_MODULES_DIR
done

sudo cp .libs/mod_paypal_ec.so $APACHE_MODULES_DIR
sudo chmod 644 $APACHE_MODULES_DIR/mod_paypal_ec.so

echo "-----------------------------------------"
echo "Would you like to set up a sample configuration for this module (Y/N)?"
read SETUP_SAMPLE_CONFIG
while [ $SETUP_SAMPLE_CONFIG != "Y" ]
do
	# forgot how to do and operations in a while condition
	# it's giving me grief.... so here's a quick hack :p
	if [ $SETUP_SAMPLE_CONFIG != "N" ]
	then
		echo "Please type Y for yes or N for no"
		read SETUP_SAMPLE_CONFIG
	else
		break
	fi
done

if [ $SETUP_SAMPLE_CONFIG == "Y" ]
then
	echo "Please provide location of your document root folder (generally it is /var/www):"
	read APACHE_DOC_ROOT
	while [ ! -d $APACHE_DOC_ROOT ]
	do
		echo "Could not find $APACHE_DOC_ROOT"
		echo "Please provide the correct location:"
		read APACHE_DOC_ROOT
	done
	
	if [ -d $APACHE_DOC_ROOT/mod_paypal_ec_sample ]
	then
		sudo rm -rf $APACHE_DOC_ROOT/mod_paypal_ec_sample
	fi

	echo "Creating sample folder with a sample book in it..."
	sudo mkdir $APACHE_DOC_ROOT/mod_paypal_ec_sample
	sudo mkdir $APACHE_DOC_ROOT/mod_paypal_ec_sample/books

	sudo touch $APACHE_DOC_ROOT/mod_paypal_ec_sample/books/mod_paypal_ec_sample_book.pdf
	echo "mod_paypal_ec_sample_book.pdf=9.99" > sample.pricelist
	sudo mv sample.pricelist $APACHE_DOC_ROOT/mod_paypal_ec_sample/books
	sudo chown -R root:root $APACHE_DOC_ROOT/mod_paypal_ec_sample/books
	
	echo "Updating paypal_ec.conf..."
	cat >> ../config/paypal_ec.conf << EOF
	
# The folder you want to secure
<Location /mod_paypal_ec_sample/books>
	# Location of the pricelist file
	Pricelist $APACHE_DOC_ROOT/mod_paypal_ec_sample/books/sample.pricelist
	
	# Do not change these settings
	AuthType ExpressCheckout
	AuthName "PayPal ExpressCheckout"
	Require valid-user
</Location>
EOF

	echo "Creating a sample merchant page with the link to the sample book..."
	cat > index.html << EOF
<html>
<body><a href="books/mod_paypal_ec_sample_book.pdf">download</a></body>
</html>
EOF
	sudo mv index.html $APACHE_DOC_ROOT/mod_paypal_ec_sample
	sudo chown root:root $APACHE_DOC_ROOT/mod_paypal_ec_sample/index.html

fi

echo "-----------------------------------------"
echo "Enabling the module..."
LIB_CURL_LOCATION=`sudo find / -name libcurl.so`
echo "LoadFile $LIB_CURL_LOCATION" > ../config/paypal_ec.load
echo "LoadModule paypal_ec_module /usr/lib/apache2/modules/mod_paypal_ec.so" >> ../config/paypal_ec.load

APACHE_MODSAVAILABLE_DIR=/etc/apache2/mods-available/

while [ ! -d $APACHE_MODSAVAILABLE_DIR ]
do
	echo "Could not find $APACHE_MODSAVAILABLE_DIR"
	echo "Please provide the correct location: "
	read APACHE_MODSAVAILABLE_DIR
done

sudo cp ../config/paypal_ec.load $APACHE_MODSAVAILABLE_DIR
sudo cp ../config/paypal_ec.conf $APACHE_MODSAVAILABLE_DIR

sudo a2enmod paypal_ec

echo "-----------------------------------------"
echo "Restarting Apache Server..."
sudo service apache2 restart

echo "-----------------------------------------"
echo "Cleaning up..."
cd $CURR_DIR
rm -rf psc-commons-master master.zip*
rm -rf out.log err.log

echo "-----------------------------------------"
echo "Installation Complete."
echo "-----------------------------------------"
