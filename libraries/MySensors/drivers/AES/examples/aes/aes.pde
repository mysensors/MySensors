#include <AES.h>
#include "./printf.h"

AES aes ;

byte *key = (unsigned char*)"0123456789010123";

byte plain[] = "Add NodeAdd NodeAdd NodeAdd NodeAdd Node";

//real iv = iv x2 ex: 01234567 = 0123456701234567
unsigned long long int my_iv = 36753562;

void setup ()
{
  Serial.begin (57600) ;
  printf_begin();
  delay(500);
  printf("\n===testng mode\n") ;
  
//  otfly_test () ;
//  otfly_test256 () ;
}

void loop () 
{
  prekey_test () ;
  delay(2000);
}

void prekey (int bits)
{
  aes.iv_inc();
  byte iv [N_BLOCK] ;
  byte plain_p[48];
  byte cipher [48] ;
  byte check [48] ;
  unsigned long ms = micros ();
  aes.set_IV(my_iv);
  aes.get_IV(iv);
  aes.do_aes_encrypt(plain,41,cipher,key,bits,iv);
  Serial.print("Encryption took: ");
  Serial.println(micros() - ms);
  ms = micros ();
  aes.set_IV(my_iv);
  aes.get_IV(iv);
  aes.do_aes_decrypt(cipher,48,check,key,bits,iv);
  Serial.print("Decryption took: ");
  Serial.println(micros() - ms);
  printf("\n\nPLAIN :");
  aes.printArray(plain,(bool)true);
  printf("\nCIPHER:");
  aes.printArray(cipher,(bool)false);
  printf("\nCHECK :");
  aes.printArray(check,(bool)true);
  printf("\nIV    :");
  aes.printArray(iv,16);
  printf("\n============================================================\n");
}

void prekey_test ()
{
  prekey (128) ;
}
