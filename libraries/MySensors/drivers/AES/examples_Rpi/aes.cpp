#include <AES.h>
#include "printf.h"

AES aes;

void prekey_test ();
void prekey (int bits, int blocks);

byte key[] = "01234567899876543210012345678998";

byte plain[] = "TESTTESTTESTTESTTESTTESTTESTTESTTESTTESTTESTTESTTESTTESTTEST";

//real iv = iv x2 ex: 01234567 = 0123456701234567
unsigned long long int my_iv = 01234567;

int main(int argc, char** argv)
{
  printf("\n===testng mode\n") ;

  for (int i=0;i<1;i++){
    prekey_test () ;
  }
}

void prekey (int bits)
{
  byte iv [N_BLOCK] ;
  byte plain_p[sizeof(plain) + (N_BLOCK - (sizeof(plain) % 16)) - 1];
  byte cipher[sizeof(plain_p)];
  aes.do_aes_encrypt(plain,sizeof(plain),cipher,key,bits);
  aes.get_IV(iv);
  aes.do_aes_dencrypt(cipher,aes.get_size(),plain_p,key,bits,iv);
  //normally u have sizeof(cipher) but if its in the same sketch you cannot determin it dynamically

  printf("\n\nPLAIN :");
  aes.printArray(plain);
  printf("\nCIPHER:");
  aes.printArray(cipher);
  printf("\nPlain2:");
  aes.printArray(plain_p);
  printf("\n============================================================\n");
}

void prekey_test ()
{
  prekey (128) ;
}
