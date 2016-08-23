Create apple.conf with the following content:

[ req ]
distinguished_name = req_name
prompt = no
[ req_name ]
CN = my-test-installer
[ extensions ]
basicConstraints=critical,CA:false
keyUsage=critical,digitalSignature
extendedKeyUsage=critical,1.2.840.113635.100.4.13
1.2.840.113635.100.6.1.14=critical,DER:0500

Generate the key:

openssl genrsa -out apple.key  2048

Create the self-signed certificate:

openssl req -x509 -new -config apple.conf -nodes -key apple.key -extensions extensions -sha256 -out apple.crt

Wrap the key and certificate into PKCS#12:

openssl pkcs12 -export -inkey apple.key -in apple.crt -out apple.p12

Import it into keychain with open apple.p12. Select "Always trust".

Use the certificate to sign installers:

productbuild --sign "my-test-installer" ...
