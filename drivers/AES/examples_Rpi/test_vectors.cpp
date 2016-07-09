#include <AES.h>
#include "printf.h"

AES aes ;

byte key [2*N_BLOCK] ;
byte plain [N_BLOCK] ;
byte iv [N_BLOCK] ;
byte cipher [N_BLOCK] ;
byte check [N_BLOCK] ;

void print_value (const char * str, byte * a, int bits);
void prekey_test_var_plaintext (int bits);
void prekey_test_var_key (int bits);
void set_bits (int bits, byte * a, int count);
void check_same (byte * a, byte * b, int bits);
void monte_carlo (int bits);

int main(int argc, char** argv)
{
  printf ("AES library test vectors") ;

  monte_carlo (128) ;

  for (int keysize = 128 ; keysize <= 256 ; keysize += 64)
    {
      prekey_test_var_plaintext (keysize) ;
      prekey_test_var_key (keysize) ;
    }
}


void prekey_test_var_plaintext (int bits)
{
  printf ("\nECB Varying Plaintext %i bits\n",bits) ;
 
  byte succ ;
  set_bits (bits, key, 0) ;  // all zero key
  succ = aes.set_key (key, bits) ;
  if (succ != SUCCESS)
    printf("Failure set_key\n") ;


  for (int bitcount = 1 ; bitcount <= 128 ; bitcount++)
    {
      printf ("COUNT = %i \n",bitcount-1);
      print_value ("KEY = ", key, bits) ;
      set_bits (128, plain, bitcount) ;
      
      print_value ("PLAINTEXT = ", plain, 128) ;
      
      succ = aes.encrypt (plain, cipher) ;
      if (succ != SUCCESS)
        printf ("Failure encrypt\n") ;

      print_value ("CIPHERTEXT = ", cipher, 128) ;
      
      succ = aes.decrypt (cipher, check) ;
      if (succ != SUCCESS)
        printf ("Failure decrypt\n") ;

      //print_value ("CHECK = ", check, 128) ;
      check_same (plain, check, 128) ;
      printf ("\n") ;
    }
}


void prekey_test_var_key (int bits)
{
  printf ("\nECB Varying key %i bits\n",bits);
  
  byte succ ;
  set_bits (128, plain, 0) ;

  for (int bitcount = 1 ; bitcount <= bits ; bitcount++)
    {
      set_bits (bits, key, bitcount) ;  // all zero key
      succ = aes.set_key (key, bits) ;
      if (succ != SUCCESS)
        printf ("Failure set_key\n") ;
      printf ("COUNT = %i\n",bitcount-1);
      print_value ("KEY = ", key, bits) ;

      print_value ("PLAINTEXT = ", plain, 128) ;

      succ = aes.encrypt (plain, cipher) ;
      if (succ != SUCCESS)
        printf ("Failure encrypt\n") ;

      print_value ("CIPHERTEXT = ", cipher, 128) ;

      succ = aes.decrypt (cipher, check) ;
      if (succ != SUCCESS)
        printf ("Failure decrypt\n") ;

      check_same (plain, check, 128) ;
      printf("\n");
    }
}

void set_bits (int bits, byte * a, int count)
{
  bits >>= 3 ;
  byte bcount = count >> 3 ;
  for (byte i = 0 ; i < bcount ; i++)
    a [i] = 0xFF ;
  if ((count & 7) != 0)
    a [bcount++] = 0xFF & (0xFF00 >> (count & 7)) ;
  for (byte i = bcount ; i < bits ; i++)
    a [i] = 0x00 ;
}

void check_same (byte * a, byte * b, int bits)
{
  bits >>= 3 ;
  for (byte i = 0 ; i < bits ; i++)
    if (a[i] != b[i])
      {
        printf ("Failure plain != check\n") ;
        return ;
      }
}

char hex[] = "0123456789abcdef" ;


void print_value (const char * str, byte * a, int bits)
{
  printf ("%s",str) ;
  bits >>= 3 ;
  for (int i = 0 ; i < bits ; i++)
    {
      byte b = a[i] ;
      printf("%c%c",hex[b >> 4],hex [b & 15]);
      //Serial.print (hex [b >> 4]) ;
      //Serial.print (hex [b & 15]) ;
    }
  printf("\n") ;
}

byte monteplain [] = 
  { 0xb9, 0x14, 0x5a, 0x76, 0x8b, 0x7d, 0xc4, 0x89, 
    0xa0, 0x96, 0xb5, 0x46, 0xf4, 0x3b, 0x23, 0x1f } ;
byte montekey []   = 
  { 0x13, 0x9a, 0x35, 0x42, 0x2f, 0x1d, 0x61, 0xde, 
    0x3c, 0x91, 0x78, 0x7f, 0xe0, 0x50, 0x7a, 0xfd } ;

void monte_carlo (int bits)
{
  printf ("\nMonte Carlo %i bits\n",bits);
  byte succ;
  for (int i = 0 ; i < 16 ; i++)
  {
    plain [i] = monteplain [i] ;
    key [i] = montekey [i] ;
  }
  for (int i = 0 ; i < 100 ; i++)
    {
     printf ("COUNT = %i\n",i);
      print_value ("KEY = ", key, bits) ;
      print_value ("PLAINTEXT = ", plain, 128) ;
      succ = aes.set_key (key, bits) ;
      for (int j = 0 ; j < 1000 ; j++)
        {
          succ = aes.encrypt (plain, cipher) ;
          aes.copy_n_bytes (plain, cipher, 16) ;
        }
      print_value ("CIPHERTEXT = ", cipher, 128) ;
      printf("\n");
      if (bits == 128)
        {
          for (byte k = 0 ; k < 16 ; k++)
            key [k] ^= cipher [k] ;
        }
      else if (bits == 192)
        {
        }
      else
        {
        }
    }
}
