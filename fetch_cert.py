import urllib.request

''' This fetches the latest godaddy g2 cert from the internet and replaces the one contained in the header if they don't matchs
'''
# Import("env")

PATH_TO_CERT_HPP = "include/cert.hpp"

CERT_HPP_TEMPLATE = """/*
    Go Daddy Root Certificate Authority - G2
    PEM (cert)
    https://certs.godaddy.com/repository/gdroot-g2.crt.pem
*/
const char* rootCACertificate = R"string_literal(
{0})string_literal";
"""

local_filename, headers = urllib.request.urlretrieve('https://certs.godaddy.com/repository/gdroot-g2.crt.pem')
cert = ""
with open(local_filename, 'r') as c:
    cert = c.read()
    if len(cert) == 0:
        exit(0)

cert_hpp_contents = ""
with open(PATH_TO_CERT_HPP, 'r') as c:
    cert_hpp_contents = c.read()
    if len(cert_hpp_contents) == 0:
        exit(0)

if cert_hpp_contents.find(cert) == -1:
    with open(PATH_TO_CERT_HPP, 'w') as c:
        c.write(CERT_HPP_TEMPLATE.format(cert))
        print("updating cert")


