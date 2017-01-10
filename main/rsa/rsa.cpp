/*
 *  The RSA public-key cryptosystem
 *
 *  Copyright (C) 2006-2010, Paul Bakker <polarssl_maintainer at polarssl.org>
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *
 *  RSA was designed by Ron Rivest, Adi Shamir and Len Adleman.
 *
 *  http://theory.lcs.mit.edu/~rivest/rsapaper.pdf
 *  http://www.cacr.math.uwaterloo.ca/hac/about/chap8.pdf
 */

#include "stdafx.h"

#include "rsa.h"
//#include "havege.h"

#include "config.h"


 // Initialize an RSA context
 //
EncryptorRSA::EncryptorRSA()
{
    memset( &ctx, 0, sizeof(ctx));
}

EncryptorRSA::~EncryptorRSA()
{ 
	Free();
}

// Free the components of an RSA key
 
void EncryptorRSA::Free()
{
    mpi_free( &ctx.RQ, &ctx.RP, &ctx.RN,
              &ctx.QP, &ctx.DQ, &ctx.DP,
              &ctx.Q,  &ctx.P,  &ctx.D,
              &ctx.E,  &ctx.N,  NULL );
}

#if defined(POLARSSL_GENPRIME)

 // Generate an RSA keypair
 //
int EncryptorRSA::GenKey(int nbits, int exponent, int (*f_rng)(void *), void *p_rng)
{
    int ret;
    mpi P1, Q1, H, G;

    if( f_rng == NULL || nbits < 128 || exponent < 3 )
        return POLARSSL_ERR_RSA_BAD_INPUT_DATA;

    mpi_init( &P1, &Q1, &H, &G, NULL );

    /*
     * find primes P and Q with Q < P so that:
     * GCD( E, (P-1)*(Q-1) ) == 1
     */
    MPI_CHK( mpi_lset( &ctx.E, exponent ) );

    do
    {
        MPI_CHK( mpi_gen_prime( &ctx.P, ( nbits + 1 ) >> 1, 0, f_rng, p_rng ) );
        MPI_CHK( mpi_gen_prime( &ctx.Q, ( nbits + 1 ) >> 1, 0, f_rng, p_rng ) );

		int cmp_val = mpi_cmp_mpi(&ctx.P, &ctx.Q);

        if (cmp_val == 0) continue;
        if (cmp_val < 0 ) mpi_swap( &ctx.P, &ctx.Q );

        MPI_CHK( mpi_mul_mpi( &ctx.N, &ctx.P, &ctx.Q ) );
        if( mpi_msb( &ctx.N ) != nbits )
            continue;

        MPI_CHK( mpi_sub_int( &P1, &ctx.P, 1 ) );		// P1 = P - 1
        MPI_CHK( mpi_sub_int( &Q1, &ctx.Q, 1 ) );		// Q1 = Q - 1
        MPI_CHK( mpi_mul_mpi( &H, &P1, &Q1 ) );			// H  = P1 * Q1;
        MPI_CHK( mpi_gcd( &G, &ctx.E, &H  ) );
    }
    while( mpi_cmp_int( &G, 1 ) != 0 );					// while (G!=1)

    MPI_CHK( mpi_inv_mod( &ctx.D , &ctx.E, &H  ) );		// D  = E^-1 mod ((P-1)*(Q-1))
    MPI_CHK( mpi_mod_mpi( &ctx.DP, &ctx.D, &P1 ) );		// DP = D mod (P - 1)
    MPI_CHK( mpi_mod_mpi( &ctx.DQ, &ctx.D, &Q1 ) );		// DQ = D mod (Q - 1)
    MPI_CHK( mpi_inv_mod( &ctx.QP, &ctx.Q, &ctx.P ) );	// QP = Q^-1 mod P

    ctx.len = (nbits + 7) >> 3;							// mpi_msb( &ctx.N )==nbits, always !

cleanup:

    mpi_free( &G, &H, &Q1, &P1, NULL );

	if (ret==0) return 0; //ok

    return( POLARSSL_ERR_RSA_KEY_GEN_FAILED | ret );
}

static int QuickRand(void *)
{
	return rand();
}

#include <time.h>

void EncryptorRSA::GenKey2()
{	
	//  Seeding the random number generator...
	//havege_state hs;
    //havege_init(&hs);

	srand(::GetTickCount() + time(NULL));

	//  Generating the RSA key [ 1024-bit ]...

	const int KEYLEN   = 1024;
	const int EXPONENT = 65537;

	int ret = this->GenKey(KEYLEN, EXPONENT, QuickRand, 0);
	//int ret = this->GenKey(KEYLEN, EXPONENT, havege_rand, &hs);
    
	if (ret != 0)
		throw RLException("EncryptorRSA:GK error=%d", (int)ret);
}


#endif // POLARSSL_GENPRIME


// Check a public RSA key
//
int EncryptorRSA::CheckPublicKey()
{
    if( !ctx.N.p || !ctx.E.p )
        return POLARSSL_ERR_RSA_KEY_CHECK_FAILED;

    if( ( ctx.N.p[0] & 1 ) == 0 || 
        ( ctx.E.p[0] & 1 ) == 0 )
        return POLARSSL_ERR_RSA_KEY_CHECK_FAILED;

    if( mpi_msb( &ctx.N ) < 128 ||
        mpi_msb( &ctx.N ) > 4096 )
        return POLARSSL_ERR_RSA_KEY_CHECK_FAILED;

    if( mpi_msb( &ctx.E ) < 2 ||
        mpi_msb( &ctx.E ) > 64 )
        return POLARSSL_ERR_RSA_KEY_CHECK_FAILED;

    return 0;
}

/*
 * Check a private RSA key
 */
int EncryptorRSA::CheckSecketKey()
{
    int ret;
    mpi PQ, DE, P1, Q1, H, I, G;

    if( ( ret = CheckPublicKey()) != 0 )
        return ret;

    if( !ctx.P.p || !ctx.Q.p || !ctx.D.p )
        return POLARSSL_ERR_RSA_KEY_CHECK_FAILED;

    mpi_init( &PQ, &DE, &P1, &Q1, &H, &I, &G, NULL );

    MPI_CHK( mpi_mul_mpi( &PQ, &ctx.P, &ctx.Q ) );
    MPI_CHK( mpi_mul_mpi( &DE, &ctx.D, &ctx.E ) );
    MPI_CHK( mpi_sub_int( &P1, &ctx.P, 1 ) );
    MPI_CHK( mpi_sub_int( &Q1, &ctx.Q, 1 ) );
    MPI_CHK( mpi_mul_mpi( &H, &P1, &Q1 ) );
    MPI_CHK( mpi_mod_mpi( &I, &DE, &H  ) );
    MPI_CHK( mpi_gcd( &G, &ctx.E, &H  ) );

    if( mpi_cmp_mpi( &PQ, &ctx.N ) == 0 &&
        mpi_cmp_int( &I, 1 ) == 0 &&
        mpi_cmp_int( &G, 1 ) == 0 )
    {
        mpi_free( &G, &I, &H, &Q1, &P1, &DE, &PQ, NULL );
        return 0;
    }

cleanup:

    mpi_free( &G, &I, &H, &Q1, &P1, &DE, &PQ, NULL );
    return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED | ret );
}

/*
 * Do an RSA public key operation
 */
int EncryptorRSA::DoPublic(const unsigned char *input, unsigned char *output )
{
    int ret, olen;
    mpi T;

    mpi_init( &T, NULL );

    MPI_CHK( mpi_read_binary( &T, input, ctx.len ) );

    if (mpi_cmp_mpi( &T, &ctx.N ) >= 0)
    {
        mpi_free( &T, NULL );
        return POLARSSL_ERR_RSA_BAD_INPUT_DATA;
    }

    olen = ctx.len;
    MPI_CHK( mpi_exp_mod( &T, &T, &ctx.E, &ctx.N, &ctx.RN ) );
    MPI_CHK( mpi_write_binary( &T, output, olen ) );

cleanup:

    mpi_free( &T, NULL );

    if( ret != 0 )
        return( POLARSSL_ERR_RSA_PUBLIC_FAILED | ret );

    return 0;
}


 // Do an RSA private key operation
 //
int EncryptorRSA::DoSecret(const unsigned char *input, unsigned char *output)
{
    int ret, olen;
    mpi T, T1, T2;

    mpi_init( &T, &T1, &T2, NULL );

    MPI_CHK( mpi_read_binary( &T, input, ctx.len ) );

    if( mpi_cmp_mpi( &T, &ctx.N ) >= 0 )
    {
        mpi_free( &T, NULL );
        return POLARSSL_ERR_RSA_BAD_INPUT_DATA;
    }

#if 0
    MPI_CHK( mpi_exp_mod( &T, &T, &ctx.D, &ctx.N, &ctx.RN ) );
#else
    /*
     * faster decryption using the CRT
     *
     * T1 = input ^ dP mod P
     * T2 = input ^ dQ mod Q
     */
    MPI_CHK( mpi_exp_mod( &T1, &T, &ctx.DP, &ctx.P, &ctx.RP ) );
    MPI_CHK( mpi_exp_mod( &T2, &T, &ctx.DQ, &ctx.Q, &ctx.RQ ) );

    /*
     * T = (T1 - T2) * (Q^-1 mod P) mod P
     */
    MPI_CHK( mpi_sub_mpi( &T, &T1, &T2 ) );
    MPI_CHK( mpi_mul_mpi( &T1, &T, &ctx.QP ) );
    MPI_CHK( mpi_mod_mpi( &T, &T1, &ctx.P ) );

    /*
     * output = T2 + T * Q
     */
    MPI_CHK( mpi_mul_mpi( &T1, &T, &ctx.Q ) );
    MPI_CHK( mpi_add_mpi( &T, &T2, &T1 ) );
#endif

    olen = ctx.len;
    MPI_CHK( mpi_write_binary( &T, output, olen ) );

cleanup:

    mpi_free( &T, &T1, &T2, NULL );

    if( ret != 0 )
        return( POLARSSL_ERR_RSA_PRIVATE_FAILED | ret );

    return 0;
}


void EncryptorRSA::ParseFromPublicKey(RLStream& key)
{
	key.SetReadPos(0);

	mpi_grow(&ctx.N, 32);  key.GetRaw(ctx.N.p,  32*4); ctx.N.s = 1;
	mpi_grow(&ctx.E, 1);   key.GetRaw(ctx.E.p,   1*4); ctx.E.s = 1;

	ctx.len = (mpi_msb(&ctx.N) + 7) >> 3;
}

void EncryptorRSA::ParseFromSecretKey(RLStream& key)
{	
	ParseFromPublicKey(key);

	mpi_grow(&ctx.D, 32);  key.GetRaw(ctx.D.p,  32*4); ctx.D.s = 1;
	mpi_grow(&ctx.P, 16);  key.GetRaw(ctx.P.p,  16*4); ctx.P.s = 1;
	mpi_grow(&ctx.Q, 16);  key.GetRaw(ctx.Q.p,  16*4); ctx.Q.s = 1;
	mpi_grow(&ctx.DP, 16); key.GetRaw(ctx.DP.p, 16*4); ctx.DP.s = 1;
	mpi_grow(&ctx.DQ, 16); key.GetRaw(ctx.DQ.p, 16*4); ctx.DQ.s = 1;
	mpi_grow(&ctx.QP, 16); key.GetRaw(ctx.QP.p, 16*4); ctx.QP.s = 1;
}

void EncryptorRSA::ExportPublicKey(RLStream& key)
{
	key.SetMinCapasity((32+1)*4);
	key.AddRaw(ctx.N.p, 32*4);
	key.AddRaw(ctx.E.p,  1*4);
}

void EncryptorRSA::ExportSecretKey(RLStream& key)
{
	key.SetMinCapasity((32+1+32+16*5)*4);
	key.AddRaw(ctx.N.p,  32*4);
	key.AddRaw(ctx.E.p,   1*4);
	key.AddRaw(ctx.D.p,  32*4);
	key.AddRaw(ctx.P.p,  16*4);
	key.AddRaw(ctx.Q.p,  16*4);
	key.AddRaw(ctx.DP.p, 16*4);
	key.AddRaw(ctx.DQ.p, 16*4);
	key.AddRaw(ctx.QP.p, 16*4);
}


// Add the message padding, then do an RSA operation
//
void EncryptorRSA::Encrypt(int mode, int  ilen, const unsigned char *input, unsigned char *output)
{
	// RSA_PKCS_V15 padding
    {
		unsigned char *p = output;
		
		int olen = ctx.len;

		if (ilen < 0 || olen < ilen + 11)
			throw RLException("EncryptorRSA:Encr error=%d", (int)POLARSSL_ERR_RSA_BAD_INPUT_DATA);

        int nb_pad = olen - 3 - ilen;

        *p++ = 0;
        *p++ = RSA_CRYPT;

        while( nb_pad-- > 0 )
        {
			do {
				*p = (unsigned char) rand();
			} while(*p == 0);
            p++;
		}
        *p++ = 0;
        memcpy( p, input, ilen );
	}

    int ret = ( mode == RSA_PUBLIC )
				? this->DoPublic(output, output)
				: this->DoSecret(output, output);

	if (ret!=0)
		throw RLException("EncryptorRSA:Encr error=%d", (int)ret);
}



// Do an RSA operation, then remove the message padding
//
void EncryptorRSA::Decrypt(int mode, int output_len, const unsigned char *input, unsigned char *output)
{
    unsigned char buf[1024];

    int ilen = ctx.len;

    if( ilen < 16 || ilen > (int) sizeof(buf))
		throw RLException("EncryptorRSA:Decr error=%d", (int)POLARSSL_ERR_RSA_BAD_INPUT_DATA);

    int ret = ( mode == RSA_PUBLIC )
          ? this->DoPublic(input, buf)
          : this->DoSecret(input, buf);

    if (ret != 0)
		throw RLException("EncryptorRSA:Decr error=%d", (int)ret);

    unsigned char *p = buf;

    // RSA_PKCS_V15 padding
    {
            if( *p++ != 0 || *p++ != RSA_CRYPT )
				throw RLException("EncryptorRSA:Decr error=%d", (int)POLARSSL_ERR_RSA_INVALID_PADDING);

            while (*p != 0)
            {
                if( p >= buf + ilen - 1 )
					throw RLException("EncryptorRSA:Decr error=%d", (int)POLARSSL_ERR_RSA_INVALID_PADDING);
                p++;
            }
            p++;
    }

	int olen = ilen - (int)(p - buf);

    //if (olen > output_max_len) throw RLException("EncryptorRSA:Decr error=%d", (int)POLARSSL_ERR_RSA_OUTPUT_TOO_LARGE);
	  if (olen != output_len)    throw RLException("EncryptorRSA:Decr error=%d", (int)POLARSSL_ERR_RSA_INVALID_PADDING);	
    
    memcpy(output, p, olen);
}



/*
// Do an RSA operation to sign the message digest
//
int rsa_pkcs1_sign( rsa_context *ctx,
                    int mode,
                    int hash_id,
                    int hashlen,
                    const unsigned char *hash,
                    unsigned char *sig )
{
    int nb_pad, olen;
    unsigned char *p = sig;

    olen = ctx->len;

    // RSA_PKCS_V15 padding
    {
            switch( hash_id )
            {
                case SIG_RSA_RAW:
                    nb_pad = olen - 3 - hashlen;
                    break;

                case SIG_RSA_MD2:
                case SIG_RSA_MD4:
                case SIG_RSA_MD5:
                    nb_pad = olen - 3 - 34;
                    break;

                case SIG_RSA_SHA1:
                    nb_pad = olen - 3 - 35;
                    break;

                case SIG_RSA_SHA224:
                    nb_pad = olen - 3 - 47;
                    break;

                case SIG_RSA_SHA256:
                    nb_pad = olen - 3 - 51;
                    break;

                case SIG_RSA_SHA384:
                    nb_pad = olen - 3 - 67;
                    break;

                case SIG_RSA_SHA512:
                    nb_pad = olen - 3 - 83;
                    break;


                default:
                    return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
            }

            if( nb_pad < 8 )
                return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

            *p++ = 0;
            *p++ = RSA_SIGN;
            memset( p, 0xFF, nb_pad );
            p += nb_pad;
            *p++ = 0;
    }

    switch( hash_id )
    {
        case SIG_RSA_RAW:
            memcpy( p, hash, hashlen );
            break;

        case SIG_RSA_MD2:
            memcpy( p, ASN1_HASH_MDX, 18 );
            memcpy( p + 18, hash, 16 );
            p[13] = 2; break;

        case SIG_RSA_MD4:
            memcpy( p, ASN1_HASH_MDX, 18 );
            memcpy( p + 18, hash, 16 );
            p[13] = 4; break;

        case SIG_RSA_MD5:
            memcpy( p, ASN1_HASH_MDX, 18 );
            memcpy( p + 18, hash, 16 );
            p[13] = 5; break;

        case SIG_RSA_SHA1:
            memcpy( p, ASN1_HASH_SHA1, 15 );
            memcpy( p + 15, hash, 20 );
            break;

        case SIG_RSA_SHA224:
            memcpy( p, ASN1_HASH_SHA2X, 19 );
            memcpy( p + 19, hash, 28 );
            p[1] += 28; p[14] = 4; p[18] += 28; break;

        case SIG_RSA_SHA256:
            memcpy( p, ASN1_HASH_SHA2X, 19 );
            memcpy( p + 19, hash, 32 );
            p[1] += 32; p[14] = 1; p[18] += 32; break;

        case SIG_RSA_SHA384:
            memcpy( p, ASN1_HASH_SHA2X, 19 );
            memcpy( p + 19, hash, 48 );
            p[1] += 48; p[14] = 2; p[18] += 48; break;

        case SIG_RSA_SHA512:
            memcpy( p, ASN1_HASH_SHA2X, 19 );
            memcpy( p + 19, hash, 64 );
            p[1] += 64; p[14] = 3; p[18] += 64; break;

        default:
            return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
    }

    return( ( mode == RSA_PUBLIC )
            ? rsa_public(  ctx, sig, sig )
            : rsa_private( ctx, sig, sig ) );
}
*/

/*
// Do an RSA operation and check the message digest
//
int rsa_pkcs1_verify( rsa_context *ctx,
                      int mode,
                      int hash_id,
                      int hashlen,
                      const unsigned char *hash,
                      unsigned char *sig )
{
    int ret, len, siglen;
    unsigned char *p, c;
    unsigned char buf[1024];

    siglen = ctx->len;

    if( siglen < 16 || siglen > (int) sizeof( buf ) )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    ret = ( mode == RSA_PUBLIC )
          ? rsa_public(  ctx, sig, buf )
          : rsa_private( ctx, sig, buf );

    if( ret != 0 )
        return( ret );

    p = buf;

    // RSA_PKCS_V15 padding
    {
            if( *p++ != 0 || *p++ != RSA_SIGN )
                return( POLARSSL_ERR_RSA_INVALID_PADDING );

            while( *p != 0 )
            {
                if( p >= buf + siglen - 1 || *p != 0xFF )
                    return( POLARSSL_ERR_RSA_INVALID_PADDING );
                p++;
            }
            p++;
    }

    len = siglen - (int)( p - buf );

    if( len == 34 )
    {
        c = p[13];
        p[13] = 0;

        if( memcmp( p, ASN1_HASH_MDX, 18 ) != 0 )
            return( POLARSSL_ERR_RSA_VERIFY_FAILED );

        if( ( c == 2 && hash_id == SIG_RSA_MD2 ) ||
            ( c == 4 && hash_id == SIG_RSA_MD4 ) ||
            ( c == 5 && hash_id == SIG_RSA_MD5 ) )
        {
            if( memcmp( p + 18, hash, 16 ) == 0 ) 
                return( 0 );
            else
                return( POLARSSL_ERR_RSA_VERIFY_FAILED );
        }
    }

    if( len == 35 && hash_id == SIG_RSA_SHA1 )
    {
        if( memcmp( p, ASN1_HASH_SHA1, 15 ) == 0 &&
            memcmp( p + 15, hash, 20 ) == 0 )
            return( 0 );
        else
            return( POLARSSL_ERR_RSA_VERIFY_FAILED );
    }
    if( ( len == 19 + 28 && p[14] == 4 && hash_id == SIG_RSA_SHA224 ) ||
        ( len == 19 + 32 && p[14] == 1 && hash_id == SIG_RSA_SHA256 ) ||
        ( len == 19 + 48 && p[14] == 2 && hash_id == SIG_RSA_SHA384 ) ||
        ( len == 19 + 64 && p[14] == 3 && hash_id == SIG_RSA_SHA512 ) )
    {
    	c = p[1] - 17;
        p[1] = 17;
        p[14] = 0;

        if( p[18] == c &&
                memcmp( p, ASN1_HASH_SHA2X, 18 ) == 0 &&
                memcmp( p + 19, hash, c ) == 0 )
            return( 0 );
        else
            return( POLARSSL_ERR_RSA_VERIFY_FAILED );
    }

    if( len == hashlen && hash_id == SIG_RSA_RAW )
    {
        if( memcmp( p, hash, hashlen ) == 0 )
            return( 0 );
        else
            return( POLARSSL_ERR_RSA_VERIFY_FAILED );
    }

    return( POLARSSL_ERR_RSA_INVALID_PADDING );
}
*/


/* // POLARSSL_SELF_TEST

#include "sha1.h"

 // Example RSA-1024 keypair, for test purposes
 
#define KEY_LEN 128

#define RSA_N   "9292758453063D803DD603D5E777D788" \
                "8ED1D5BF35786190FA2F23EBC0848AEA" \
                "DDA92CA6C3D80B32C4D109BE0F36D6AE" \
                "7130B9CED7ACDF54CFC7555AC14EEBAB" \
                "93A89813FBF3C4F8066D2D800F7C38A8" \
                "1AE31942917403FF4946B0A83D3D3E05" \
                "EE57C6F5F5606FB5D4BC6CD34EE0801A" \
                "5E94BB77B07507233A0BC7BAC8F90F79"

#define RSA_E   "10001"

#define RSA_D   "24BF6185468786FDD303083D25E64EFC" \
                "66CA472BC44D253102F8B4A9D3BFA750" \
                "91386C0077937FE33FA3252D28855837" \
                "AE1B484A8A9A45F7EE8C0C634F99E8CD" \
                "DF79C5CE07EE72C7F123142198164234" \
                "CABB724CF78B8173B9F880FC86322407" \
                "AF1FEDFDDE2BEB674CA15F3E81A1521E" \
                "071513A1E85B5DFA031F21ECAE91A34D"

#define RSA_P   "C36D0EB7FCD285223CFB5AABA5BDA3D8" \
                "2C01CAD19EA484A87EA4377637E75500" \
                "FCB2005C5C7DD6EC4AC023CDA285D796" \
                "C3D9E75E1EFC42488BB4F1D13AC30A57"

#define RSA_Q   "C000DF51A7C77AE8D7C7370C1FF55B69" \
                "E211C2B9E5DB1ED0BF61D0D9899620F4" \
                "910E4168387E3C30AA1E00C339A79508" \
                "8452DD96A9A5EA5D9DCA68DA636032AF"

#define RSA_DP  "C1ACF567564274FB07A0BBAD5D26E298" \
                "3C94D22288ACD763FD8E5600ED4A702D" \
                "F84198A5F06C2E72236AE490C93F07F8" \
                "3CC559CD27BC2D1CA488811730BB5725"

#define RSA_DQ  "4959CBF6F8FEF750AEE6977C155579C7" \
                "D8AAEA56749EA28623272E4F7D0592AF" \
                "7C1F1313CAC9471B5C523BFE592F517B" \
                "407A1BD76C164B93DA2D32A383E58357"

#define RSA_QP  "9AE7FBC99546432DF71896FC239EADAE" \
                "F38D18D2B2F0E2DD275AA977E2BF4411" \
                "F5A3B2A5D33605AEBBCCBA7FEB9F2D2F" \
                "A74206CEC169D74BF5A8C50D6F48EA08"

#define PT_LEN  24
#define RSA_PT  "\xAA\xBB\xCC\x03\x02\x01\x00\xFF\xFF\xFF\xFF\xFF" \
                "\x11\x22\x33\x0A\x0B\x0C\xCC\xDD\xDD\xDD\xDD\xDD"


 // Checkup routine
 //
int rsa_self_test( int verbose )
{
    int len;
    rsa_context rsa;
    unsigned char sha1sum[20];
    unsigned char rsa_plaintext[PT_LEN];
    unsigned char rsa_decrypted[PT_LEN];
    unsigned char rsa_ciphertext[KEY_LEN];

    memset( &rsa, 0, sizeof( rsa_context ) );

    rsa.len = KEY_LEN;
    mpi_read_string( &rsa.N , 16, RSA_N  );
    mpi_read_string( &rsa.E , 16, RSA_E  );
    mpi_read_string( &rsa.D , 16, RSA_D  );
    mpi_read_string( &rsa.P , 16, RSA_P  );
    mpi_read_string( &rsa.Q , 16, RSA_Q  );
    mpi_read_string( &rsa.DP, 16, RSA_DP );
    mpi_read_string( &rsa.DQ, 16, RSA_DQ );
    mpi_read_string( &rsa.QP, 16, RSA_QP );

    if( verbose != 0 )
        printf( "  RSA key validation: " );

    if( rsa_check_pubkey(  &rsa ) != 0 ||
        rsa_check_privkey( &rsa ) != 0 )
    {
        if( verbose != 0 )
            printf( "failed\n" );

        return( 1 );
    }

    if( verbose != 0 )
        printf( "passed\n  PKCS#1 encryption : " );

    memcpy( rsa_plaintext, RSA_PT, PT_LEN );

    if( rsa_pkcs1_encrypt( &rsa, RSA_PUBLIC, PT_LEN, rsa_plaintext, rsa_ciphertext ) != 0 )
    {
        if( verbose != 0 )
            printf( "failed\n" );

        return( 1 );
    }

    if( verbose != 0 )
        printf( "passed\n  PKCS#1 decryption : " );

    if( rsa_pkcs1_decrypt( &rsa, RSA_PRIVATE, &len,
                           rsa_ciphertext, rsa_decrypted,
			   sizeof(rsa_decrypted) ) != 0 )
    {
        if( verbose != 0 )
            printf( "failed\n" );

        return( 1 );
    }

    if( memcmp( rsa_decrypted, rsa_plaintext, len ) != 0 )
    {
        if( verbose != 0 )
            printf( "failed\n" );

        return( 1 );
    }

    if( verbose != 0 )
        printf( "passed\n  PKCS#1 data sign  : " );

    sha1( rsa_plaintext, PT_LEN, sha1sum );

    if( rsa_pkcs1_sign( &rsa, RSA_PRIVATE, SIG_RSA_SHA1, 20,
                        sha1sum, rsa_ciphertext ) != 0 )
    {
        if( verbose != 0 )
            printf( "failed\n" );

        return( 1 );
    }

    if( verbose != 0 )
        printf( "passed\n  PKCS#1 sig. verify: " );

    if( rsa_pkcs1_verify( &rsa, RSA_PUBLIC, SIG_RSA_SHA1, 20,
                          sha1sum, rsa_ciphertext ) != 0 )
    {
        if( verbose != 0 )
            printf( "failed\n" );

        return( 1 );
    }

    if( verbose != 0 )
        printf( "passed\n\n" );

    Free();

    return( 0 );
}
*/

/*
int ParseFromPrivateTextFile()
{
	int ret;
    if ((f = fopen("rsa_priv.txt", "rb")) == NULL)
        return 1;
	
    if ((ret = mpi_read_file(&rsa.N , 16, f)) != 0 ||
        (ret = mpi_read_file(&rsa.E , 16, f)) != 0 ||
        (ret = mpi_read_file(&rsa.D , 16, f)) != 0 ||
        (ret = mpi_read_file(&rsa.P , 16, f)) != 0 ||
        (ret = mpi_read_file(&rsa.Q , 16, f)) != 0 ||
        (ret = mpi_read_file(&rsa.DP, 16, f)) != 0 ||
        (ret = mpi_read_file(&rsa.DQ, 16, f)) != 0 ||
        (ret = mpi_read_file(&rsa.QP, 16, f)) != 0)
    {
        printf("mpi_read_file returned %d\n\n", ret);
		fclose(f);
        return 1;
    }

	// Длина подписи также будет длиной rsa.len !!!
    rsa.len = (mpi_msb(&rsa.N) + 7) >> 3;
    fclose(f);
}

int ExportKeys()
{
	//printf(" ok\n  . Exporting the public  key in rsa_pub.txt....");

	if ((fpub = fopen( "rsa_pub.txt", "wb+")) == NULL)
	{
		rsa_free(&rsa); 
		printf( " failed\n  ! could not open rsa_pub.txt for writing\n\n" );
		return 0;
    }

	if ((ret = mpi_write_file("N = ", &rsa.N, 16, fpub)) != 0 ||
        (ret = mpi_write_file("E = ", &rsa.E, 16, fpub)) != 0)
	{
		rsa_free(&rsa);
		fclose(fpub);
		printf(" failed\n  ! mpi_write_file returned %d\n\n", ret );
		return 0;
    }

	fclose(fpub);

    printf(" ok\n  . Exporting the private key in rsa_priv.txt...");
    fflush(stdout);

    if ((fpriv = fopen("rsa_priv.txt", "wb+")) == NULL)
    {
		rsa_free(&rsa);
		printf(" failed\n  ! could not open rsa_priv.txt for writing\n");
		return 0;
    }

    if ((ret = mpi_write_file("N = " , &rsa.N , 16, fpriv)) != 0 ||
        (ret = mpi_write_file("E = " , &rsa.E , 16, fpriv)) != 0 ||
        (ret = mpi_write_file("D = " , &rsa.D , 16, fpriv)) != 0 ||
        (ret = mpi_write_file("P = " , &rsa.P , 16, fpriv)) != 0 ||
        (ret = mpi_write_file("Q = " , &rsa.Q , 16, fpriv)) != 0 ||
        (ret = mpi_write_file("DP = ", &rsa.DP, 16, fpriv)) != 0 ||
        (ret = mpi_write_file("DQ = ", &rsa.DQ, 16, fpriv)) != 0 ||
        (ret = mpi_write_file("QP = ", &rsa.QP, 16, fpriv)) != 0)
    {
		fclose(fpriv);
		rsa_free(&rsa);
        printf(" failed\n  ! mpi_write_file returned %d\n\n", ret);
		return 0;
    } 

	fclose(fpriv);
}
*/


