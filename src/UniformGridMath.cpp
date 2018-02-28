#include "UniformGridMath.hpp"

void UniformGridMath::ComputeJacobian( UniformGrid< glm::mat3 > & jacobian , const UniformGrid< glm::vec3 > & vec ) {
    const glm::vec3      spacing                 = vec.GetCellSpacing() ;
    // Avoid divide-by-zero when z size is effectively 0 (for 2D domains)
    const glm::vec3      reciprocalSpacing( 1.0f / spacing.x , 1.0f / spacing.y , spacing.z > FLT_EPSILON ? 1.0f / spacing.z : 0.0f ) ;
    const glm::vec3      halfReciprocalSpacing( 0.5f * reciprocalSpacing ) ;
    const size_t  dims[3]                 = { vec.GetNumPoints( 0 )   , vec.GetNumPoints( 1 )   , vec.GetNumPoints( 2 )   } ;
    const size_t  dimsMinus1[3]           = { vec.GetNumPoints( 0 )-1 , vec.GetNumPoints( 1 )-1 , vec.GetNumPoints( 2 )-1 } ;
    const size_t  numXY                   = dims[0] * dims[1] ;
    size_t        index[3] ;

// Reusable vars declared as macros to keep code compact
// TODO: Reuse vars, as it is more space-efficient.
#define ASSIGN_Z_OFFSETS                                    \
    const size_t offsetZM = numXY * ( index[2] - 1 ) ;    \
    const size_t offsetZ0 = numXY *   index[2]       ;    \
    const size_t offsetZP = numXY * ( index[2] + 1 ) ;

#define ASSIGN_YZ_OFFSETS                                                   \
    const size_t offsetYMZ0 = dims[ 0 ] * ( index[1] - 1 ) + offsetZ0 ;   \
    const size_t offsetY0Z0 = dims[ 0 ] *   index[1]       + offsetZ0 ;   \
    const size_t offsetYPZ0 = dims[ 0 ] * ( index[1] + 1 ) + offsetZ0 ;   \
    const size_t offsetY0ZM = dims[ 0 ] *   index[1]       + offsetZM ;   \
    const size_t offsetY0ZP = dims[ 0 ] *   index[1]       + offsetZP ;

#define ASSIGN_XYZ_OFFSETS                                      \
    const size_t offsetX0Y0Z0 = index[0]     + offsetY0Z0 ;   \
    const size_t offsetXMY0Z0 = index[0] - 1 + offsetY0Z0 ;   \
    const size_t offsetXPY0Z0 = index[0] + 1 + offsetY0Z0 ;   \
    const size_t offsetX0YMZ0 = index[0]     + offsetYMZ0 ;   \
    const size_t offsetX0YPZ0 = index[0]     + offsetYPZ0 ;   \
    const size_t offsetX0Y0ZM = index[0]     + offsetY0ZM ;   \
    const size_t offsetX0Y0ZP = index[0]     + offsetY0ZP ;

    // Compute derivatives for interior (i.e. away from boundaries).
    for( index[2] = 1 ; index[2] < dimsMinus1[2] ; ++ index[2] )
    {
        ASSIGN_Z_OFFSETS ;
        for( index[1] = 1 ; index[1] < dimsMinus1[1] ; ++ index[1] )
        {
            ASSIGN_YZ_OFFSETS ;
            for( index[0] = 1 ; index[0] < dimsMinus1[0] ; ++ index[0] )
            {
                ASSIGN_XYZ_OFFSETS ;

                glm::mat3 & rMatrix = jacobian[ offsetX0Y0Z0 ] ;
                /// Compute d/dx
                rMatrix[0] = ( vec[ offsetXPY0Z0 ] - vec[ offsetXMY0Z0 ] ) * halfReciprocalSpacing.x ;
                /// Compute d/dy
                rMatrix[1] = ( vec[ offsetX0YPZ0 ] - vec[ offsetX0YMZ0 ] ) * halfReciprocalSpacing.y ;
                /// Compute d/dz
                rMatrix[2] = ( vec[ offsetX0Y0ZP ] - vec[ offsetX0Y0ZM ] ) * halfReciprocalSpacing.z ;
            }
        }
    }

// Reusable code block
// Compute derivatives for boundaries: 6 faces of box.
// In some situations, these macros compute extraneous data.
#define COMPUTE_FINITE_DIFF                                                                                                              \
    glm::mat3 & rMatrix = jacobian[ offsetX0Y0Z0 ] ;                                                                                     \
    if( index[0] == 0 )                     { rMatrix[0] = ( vec[ offsetXPY0Z0 ] - vec[ offsetX0Y0Z0 ] ) * reciprocalSpacing.x ;     }   \
    else if( index[0] == dimsMinus1[0] )    { rMatrix[0] = ( vec[ offsetX0Y0Z0 ] - vec[ offsetXMY0Z0 ] ) * reciprocalSpacing.x ;     }   \
    else                                    { rMatrix[0] = ( vec[ offsetXPY0Z0 ] - vec[ offsetXMY0Z0 ] ) * halfReciprocalSpacing.x ; }   \
    if( index[1] == 0 )                     { rMatrix[1] = ( vec[ offsetX0YPZ0 ] - vec[ offsetX0Y0Z0 ] ) * reciprocalSpacing.y ;     }   \
    else if( index[1] == dimsMinus1[1] )    { rMatrix[1] = ( vec[ offsetX0Y0Z0 ] - vec[ offsetX0YMZ0 ] ) * reciprocalSpacing.y ;     }   \
    else                                    { rMatrix[1] = ( vec[ offsetX0YPZ0 ] - vec[ offsetX0YMZ0 ] ) * halfReciprocalSpacing.y ; }   \
    if( index[2] == 0 )                     { rMatrix[2] = ( vec[ offsetX0Y0ZP ] - vec[ offsetX0Y0Z0 ] ) * reciprocalSpacing.z ;     }   \
    else if( index[2] == dimsMinus1[2] )    { rMatrix[2] = ( vec[ offsetX0Y0Z0 ] - vec[ offsetX0Y0ZM ] ) * reciprocalSpacing.z ;     }   \
    else                                    { rMatrix[2] = ( vec[ offsetX0Y0ZP ] - vec[ offsetX0Y0ZM ] ) * halfReciprocalSpacing.z ; }

        // Compute derivatives for -X boundary.
    index[0] = 0 ;
    for( index[2] = 0 ; index[2] < dims[2] ; ++ index[2] )
    {
        ASSIGN_Z_OFFSETS ;
        for( index[1] = 0 ; index[1] < dims[1] ; ++ index[1] )
        {
            ASSIGN_YZ_OFFSETS ;
            {
                ASSIGN_XYZ_OFFSETS ;
                COMPUTE_FINITE_DIFF ;
            }
        }
    }

    // Compute derivatives for -Y boundary.
    index[1] = 0 ;
    for( index[2] = 0 ; index[2] < dims[2] ; ++ index[2] )
    {
        ASSIGN_Z_OFFSETS ;
        {
            ASSIGN_YZ_OFFSETS ;
            for( index[0] = 0 ; index[0] < dims[0] ; ++ index[0] )
            {
                ASSIGN_XYZ_OFFSETS ;
                COMPUTE_FINITE_DIFF ;
            }
        }
    }

    // Compute derivatives for -Z boundary.
    index[2] = 0 ;
    {
        ASSIGN_Z_OFFSETS ;
        for( index[1] = 0 ; index[1] < dims[1] ; ++ index[1] )
        {
            ASSIGN_YZ_OFFSETS ;
            for( index[0] = 0 ; index[0] < dims[0] ; ++ index[0] )
            {
                ASSIGN_XYZ_OFFSETS ;
                COMPUTE_FINITE_DIFF ;
            }
        }
    }

    // Compute derivatives for +X boundary.
    index[0] = dimsMinus1[0] ;
    for( index[2] = 0 ; index[2] < dims[2] ; ++ index[2] )
    {
        ASSIGN_Z_OFFSETS ;
        for( index[1] = 0 ; index[1] < dims[1] ; ++ index[1] )
        {
            ASSIGN_YZ_OFFSETS ;
            {
                ASSIGN_XYZ_OFFSETS ;
                COMPUTE_FINITE_DIFF ;
            }
        }
    }


    // Compute derivatives for +Y boundary.
    index[1] = dimsMinus1[1] ;
    for( index[2] = 0 ; index[2] < dims[2] ; ++ index[2] )
    {
        ASSIGN_Z_OFFSETS ;
        {
            ASSIGN_YZ_OFFSETS ;
            for( index[0] = 0 ; index[0] < dims[0] ; ++ index[0] )
            {
                ASSIGN_XYZ_OFFSETS ;
                COMPUTE_FINITE_DIFF ;
            }
        }
    }

    // Compute derivatives for +Z boundary.
    index[2] = dimsMinus1[2] ;
    {
        ASSIGN_Z_OFFSETS ;
        for( index[1] = 0 ; index[1] < dims[1] ; ++ index[1] )
        {
            ASSIGN_YZ_OFFSETS ;
            for( index[0] = 0 ; index[0] < dims[0] ; ++ index[0] )
            {
                ASSIGN_XYZ_OFFSETS ;
                COMPUTE_FINITE_DIFF ;
            }
        }
    }

#undef COMPUTE_FINITE_DIFF
#undef ASSIGN_XYZ_OFFSETS
#undef ASSIGN_YZ_OFFSETS
#undef ASSIGN_Z_OFFSETS

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
