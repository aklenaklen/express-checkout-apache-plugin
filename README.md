# Apache Module mod_paypal_ec
    Description:		Provides a way to make "paid URIs" using PayPal Express Checkout
    Status: 			External
    Module Identifier:	paypal_ec_module
    Source File:		mod_paypal_ec.c
    Compatibility: 		Available in Apache 2 and later 

> IMPORTANT NOTICE:

> This plugin is currently being provided "just as a sample" to show how Apache Web Server can be set up to automatically trigger PayPal Express Checkout flow for certain URIs. Pending issues in this plugin are being tracked via github issues.

## Summary
This module uses the concept of basic authentication to make URIs accessible only after the user has "paid". When a user clicks on a URI such as a download link of a digital resource, he/she is redirected to PayPal and is required to complete payment. Once payment is successful, the digital resource is then made available to the user.

The module can be configured on "per digital resource" basis. In other words, each digital resource can have its own unique price.

### Usage
The instructions below will help you create a sample URI accessible only after the user has paid using PayPal Sandbox. Once you understand the concept, you can then apply it to your actual URIs and use Production PayPal Settings.

#### Installation

> This module works with Apache 2 and later.

##### Ubuntu
Simply copy the ubuntu-install.sh in your local ubuntu machine (provided in the script folder) and run it. Don't forgot to `chmod +x` it!. Here's a sample output of the script:

    $ ./ubuntu-install.sh
    -----------------------------------------
    Installing apache2-threaded-dev and libcurl4-openssl-dev...
    -----------------------------------------
    Downloading latest mod_paypal_ec from github.paypal.com...
    Unzipping tar ball...
    -----------------------------------------
    Compiling the module...
    /usr/share/apr-1.0/build/libtool --silent --mode=compile --tag=disable-static x86_64-linux-gnu-gcc -prefer-pic -DLINUX=2 -D_FORTIFY_SOURCE=2 -D_GNU_SOURCE -D_REENTRANT -I/usr/include/apr-1.0 -I/usr/include/openssl -I/usr/include/xmltok -pthread     -I/usr/include/apache2  -I/usr/include/apr-1.0   -I/usr/include/apr-1.0   -c -o mod_paypal_ec.lo mod_paypal_ec.c && touch mod_paypal_ec.slo
    /usr/share/apr-1.0/build/libtool --silent --mode=link --tag=disable-static x86_64-linux-gnu-gcc -o mod_paypal_ec.la  -rpath /usr/lib/apache2/modules -module -avoid-version    mod_paypal_ec.lo
    -----------------------------------------
    Would you like to set up a sample configuration for this module (Y/N)?
    Y
    Please provide location of your document root folder (generally it is /var/www):
    /var/www
    Creating sample folder with a sample book in it...
    Updating paypal_ec.conf...
    Creating a sample merchant page with the link to the sample book...
    -----------------------------------------
    Enabling the module...
    Module paypal_ec already enabled
    -----------------------------------------
    Restarting Apache Server...
     * Restarting web server apache2                                                                                                                  
                                                                                                                                                  [ OK ]
    -----------------------------------------
    Cleaning up...
    -----------------------------------------
    Installation Complete.
    -----------------------------------------

Now simply go to `http://<your-server>/mod_paypal_ec_sample`. You'll see a "download" link. Click it and it will take you to PayPal sandbox. It will show you the pdf only after you complete the payment.

##### Other Linux Environments

    1. Install apache2-threaded-dev and libcurl4-openssl-dev modules.
    
    2. Create a temp directory (that you can delete after installation is complete). Let's call it /usr/tmp
    $ mkdir /usr/tmp
    $ cd /usr/tmp
    
    2. Download latest psc-commons from `https://github.paypal.com/ProfessionalServiceEngineering/psc-commons/archive/master.zip`
    wget https://github.paypal.com/ProfessionalServiceEngineering/psc-commons/archive/master.zip
    
    3. Unzip master.zip
    $ unzip master.zip
    
    4. Go to the directory psc-commons-master/mod_paypal_ec/src and compile mod_paypal_ec.c
    $ cd psc-commons-master/mod_paypal_ec/src
    $ apxs2 -c mod_paypal_ec.c 
    
    5. This will generate the libraries under ".libs" folder. Copy .libs/mod_paypal_ec.so to your Apache Modules directory (usually it is /usr/lib/apache2/modules)
    $ sudo cp .libs/mod_paypal_ec.so /usr/lib/apache2/modules/
    
    6. Go to your Apache Document Root (usually /var/www) and create a folder called mod_paypal_ec_sample and a folder called books under it
    $ sudo mkdir /var/www/mod_paypal_ec_sample
    $ sudo mkdir /var/www/mod_paypal_ec_sample/books
    
    7. Add a sample book in it (mod_paypal_ec_sample/books/mod_paypal_ec_sample_book.pdf)
    $ sudo touch /var/www/mod_paypal_ec_sample/books/mod_paypal_ec_sample_book.pdf
    
    8. Create a file "sample.pricelist" and move it under books folder
    $ echo "mod_paypal_ec_sample_book.pdf=9.99" > sample.pricelist
    $ sudo mv sample.pricelist /var/www/mod_paypal_ec_sample/books
    $ sudo chown -R root:root /var/www/mod_paypal_ec_sample/books
    
    9. Add the following lines to ../config/paypal_ec.conf:
    # The folder you want to secure
    <Location /mod_paypal_ec_sample/books>
    	# Location of the pricelist file
    	Pricelist /var/www/mod_paypal_ec_sample/books/sample.pricelist
    	
    	# Do not change these settings
    	AuthType ExpressCheckout
    	AuthName "PayPal ExpressCheckout"
    	Require valid-user
    </Location>
    
    10. Create a sample index.html under /var/www/mod_paypal_ec_sample folder and make sure it's owned by root
    <html>
    <body><a href="books/mod_paypal_ec_sample_book.pdf">download</a></body>
    </html>
    
    $ sudo chown root:root /var/www/mod_paypal_ec_sample/index.html
    
    11. Create paypal_ec.load file under ../config folder. Put the following lines in it:
    LoadFile <location of your libcurl.so file>
    LoadModule paypal_ec_module /usr/lib/apache2/modules/mod_paypal_ec.so
    
    12. Copy paypal_ec.load and paypal_ec.conf files to apache mods-available folder
    $ sudo cp ../config/paypal_ec.load /etc/apache2/mods-available/
    $ sudo cp ../config/paypal_ec.conf /etc/apache2/mods-available/
    
    13. Enable module
    $ sudo a2enmod paypal_ec
    
    14. Restart apache
    $ sudo service apache2 restart

Now simply go to `http://<your-server>/mod_paypal_ec_sample`. You'll see a "download" link. Click it and it will take you to PayPal sandbox. It will show you the pdf only after you complete the payment.
    
#### Configuration
This module can be configured by updating the /etc/apache2/mods-available/paypal_ec.conf file. Here are the settings in this file:

    +-----------------------+--------------------------------------------------------------------------------------------+
    | Parameter             | Description                                                                                |
    +-----------------------+--------------------------------------------------------------------------------------------+
    | ApiEndPoint           | The PayPal API Endpoint.                                                                   |
    |                       | For production use: https://api-3t.paypal.com/nvp                                          |
    |                       | For sandbox use: https://api-3t.sandbox.paypal.com/nvp                                     |
    +-----------------------+--------------------------------------------------------------------------------------------+
    | ApiUserName           | Your API Username                                                                          |
    +-----------------------+--------------------------------------------------------------------------------------------+
    | ApiPassword           | Your API Password                                                                          |
    +-----------------------+--------------------------------------------------------------------------------------------+
    | ApiSignature          | Your API Signature                                                                         |
    +-----------------------+--------------------------------------------------------------------------------------------+
    | ApiVersion            | PayPal API Version                                                                         |
    +-----------------------+--------------------------------------------------------------------------------------------+
    | ExpressCheckoutWebUrl | Express Checkout Redirect URL.                                                             |
    |                       | For production use https://www.paypal.com/cgi-bin/webscr?cmd=_express-checkout&token=      |
    |                       | For sandbox use https://www.sandbox.paypal.com/cgi-bin/webscr?cmd=_express-checkout&token= |
    +-----------------------+--------------------------------------------------------------------------------------------+
    | CurrencyCode          | Currency code to be used                                                                   |
    +-----------------------+--------------------------------------------------------------------------------------------+
    | <Location block>      | The folder location to be made available only once the user has paid. A sample would look  |
    |                       | like:                                                                                      |
    |                       |    <Location /mod_paypal_ec_sample/books>                                                  |
    |                       |      # Location of the pricelist file                                                      |
    |                       |      Pricelist /var/www/mod_paypal_ec_sample/books/sample.pricelist                        |
    |                       |                                                                                            |
    |                       |      # Do not change these settings                                                        |
    |                       |      AuthType ExpressCheckout                                                              |
    |                       |      AuthName "PayPal ExpressCheckout"                                                     |
    |                       |      Require valid-user                                                                    |
    |                       |    </Location>                                                                             |
    +-----------------------+--------------------------------------------------------------------------------------------+
    
#### Debugging
TBD
