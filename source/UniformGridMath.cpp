#include "UniformGridMath.hpp"

void UniformGridMath::ComputeJacobian( UniformGrid< glm::mat3 > & jacobian , const UniformGrid< glm::vec3 > & vec ) {

}

void UniformGridMath::ComputeCurlFromJacobian( UniformGrid< glm::vec3 > & curl , const UniformGrid< glm::mat3 > & jacobian ) {
	const size_t  dims[3]     = { jacobian.GetNumPoints( 0 ) , jacobian.GetNumPoints( 1 ) , jacobian.GetNumPoints( 2 ) } ;
	const size_t  numXY       = dims[0] * dims[1] ;
	size_t        index[3] ;

    // Compute curl from Jacobian
	for( index[2] = 0 ; index[2] < dims[2] ; ++ index[2] )
	{
		const size_t offsetZ = numXY * index[2]       ;
		for( index[1] = 0 ; index[1] < dims[1] ; ++ index[1] )
		{
			const size_t offsetYZ = dims[ 0 ] * index[1] + offsetZ ;
			for( index[0] = 0 ; index[0] < dims[0] ; ++ index[0] )
			{
				const size_t offsetXYZ = index[0] + offsetYZ ;
				const glm::mat3 & j     = jacobian[ offsetXYZ ] ;
				glm::vec3        & rCurl = curl[ offsetXYZ ] ;
                // Meaning of j[i][k] is the derivative of the kth component with respect to i, i.e. di/dk.
				// rCurl = glm::vec3( j.y.z - j.z.y , j.z.x - j.x.z , j.x.y - j.y.x ) ;
				rCurl = glm::vec3( j[1][2] - j[2][1] , j[2][0] - j[0][2] , j[0][1] - j[1][0] ) ;

			}
		}
	}
}