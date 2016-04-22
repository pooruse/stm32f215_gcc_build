#ifndef _IKV_CRYPTO_H_
#define _IKV_CRYPTO_H_

#define DOUBLE_ARRAY_LEN(A) ((2*(A)+31)/32)

#define GF2_131 (131) /** sizes of finite fields GF(2^n) */
#define GF2_163 (163)
#define GF2_193 (193)
#define MAX_DEGREE (193)
#define ARRAY_LEN(A) (((A)+31)/32)


/************************* Type Define ***************************/
/** data type for points on elliptic curve */
/** data type for elements in GF(2^n) and GF(p) */
typedef uint32_t dwordvec_t[ARRAY_LEN(MAX_DEGREE)];


typedef struct {
    dwordvec_t x_coord;
    dwordvec_t y_coord;
} eccpoint_t;


/** data type for elliptic curve parameters */
/** data type for signature */
typedef struct {
    dwordvec_t r_value; // r value of signature 
    dwordvec_t s_value; // s value of signature
} signature_t;

typedef uint32_t mac_t[3];  
  
typedef struct {
	unsigned int degree;    /* extension degree n of the finite field GF(2^n) */
	uint32_t *coeff_a;      /* parameter a of the elliptic curve y^2+xy = x^3+ax^2+b */
	uint32_t *coeff_sqrt_b; /* square root of parameter b of the elliptic curve y^2+xy = x^3+ax^2+b */
	uint32_t *base_point_x; /* x-coordinate of the base point */
	uint32_t *base_point_y; /* y-coordinate of the base point */
	uint32_t *order;        /* prime order of the base point */
} curve_parameter_t;

typedef void (*func2_pt)(uint32_t *, const uint32_t *);
typedef void (*func3_pt)(uint32_t *, const uint32_t *, const uint32_t *);

typedef uint32_t double_dwordvec_t[DOUBLE_ARRAY_LEN(MAX_DEGREE)];

#define MAX_ODC_LEN (96u) // 256 x 3 bits (in bytes)
#define ODC_193_LEN (56u) // in bytes
#define ODC_PK163_LEN (21u) // in bytes
#define ODC_193_PK163_LEN (ODC_193_LEN + ODC_PK163_LEN) // 56 + 21 bytes 

typedef struct
{
	unsigned int	degree;
	//uint32_t        *order;
	uint32_t  		*irred_polynomial;
		
	// gf2n operators
	func2_pt 		dwordvec_l_shift;
	func2_pt		dwordvec_copy;	
	func2_pt		sum;
	func2_pt		square;
	func3_pt		add;
	func3_pt 		mul;

} ikv_crypto_ecc_gf2n_context;

typedef struct 
{
	// curve parameters
	curve_parameter_t 			*curve_param;
	eccpoint_t					base_point;
	uint32_t        			*order;	
	ikv_crypto_ecc_gf2n_context gf2n;
	
} ikv_crypto_ecc_context;

int dwordvec_iszero(unsigned int degree, const dwordvec_t el);

ikv_crypto_ecc_context* ecc_init(const curve_parameter_t *p);
void mont_ecc_mul (ikv_crypto_ecc_context *c,
				   dwordvec_t A, dwordvec_t B, dwordvec_t C, dwordvec_t D,
                   const dwordvec_t point_x, const dwordvec_t scalar);

void ecc_mul_projective (ikv_crypto_ecc_context *c,
 						 dwordvec_t X, dwordvec_t Y, dwordvec_t Z, 
						 const eccpoint_t *p, const dwordvec_t scalar);
int gf2n_divide (ikv_crypto_ecc_gf2n_context *c, dwordvec_t t1, 
				 const dwordvec_t op1, const dwordvec_t op2);
int ecdsa_verify(dwordvec_t, dwordvec_t, 
                 uint8_t *, eccpoint_t *, curve_parameter_t *);
void mac_algorithm_80 (mac_t mac_value, 
			   		   mac_t data, mac_t session_key);
void sha256 (uint8_t *hash_value, 
			 const uint8_t *input_data, const uint32_t input_length);

#endif				// _IKV_CRYPTO_H_ 

