/*
  * NextPrime
  *
  * Return the first prime number larger than the integer
  * given as a parameter. The integer must be positive.
  */
#define PRIME_FALSE      0
#define PRIME_TRUE       1
int next_prime( int inval )
{
	int perhapsprime;
	int testfactor;
	int found; 
	if(inval<3) {
/* Constant to help readability. */
/* Constant to help readability. */
/* Holds a tentative prime while we check it. */
/* Holds various factors for which we test perhapsprime. */
/* Flag, false until we find a prime. */
/* Initial sanity check of parameter. */
		if(inval <= 0) return(1); /* Return 1 for zero or negative input. */
		if(inval == 1) return(2); /* Easy special case. */
		if(inval == 2) return(3); /* Easy special case. */
	} else {
  /* Testing an even number for primeness is pointless, since
    * all even numbers are divisible by 2. Therefore, we make sure
    * that perhapsprime is larger than the parameter, and odd. */
		perhapsprime = ( inval + 1 ) | 1 ;
	}
/* While prime not found, loop. */
	for( found = PRIME_FALSE; found != PRIME_TRUE; perhapsprime += 2 ) {
  /* Check factors from 3 up to perhapsprime/2. */
		for( testfactor = 3; testfactor <= (perhapsprime >> 1); testfactor += 1 ) {
			found = PRIME_TRUE;       /* Assume we will find a prime. */
			if( (perhapsprime % testfactor) == 0 ) {                    /* If testfactor divides perhapsprime... */
				found = PRIME_FALSE;    /* ...then, perhapsprime was non-prime. */
				goto check_next_prime; /* Break the inner loop, go test a new perhapsprime. */
			}
		}
	
		check_next_prime:; /* This label is used to break the inner loop. */
		if( found == PRIME_TRUE ) {
	   	return( perhapsprime ); /* Return the prime we found. */
		}
	}
	return( perhapsprime );
}
/* When the loop ends,
   perhapsprime is a real prime. */
