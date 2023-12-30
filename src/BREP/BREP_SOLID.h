#ifndef __BREP_SHELL__
#define __BREP_SHELL__

#include "BREP/BREP_FACE.h"

/**
The Topological Data Structure
An edge-based Boundary Representation of solids, similar to the winged-edge data structure.
References:
- Kevin Weiler, "Edge-based Data Structures for Solid Modeling in
  Curved-Surface Environments", IEEE Computer graphics and Applications,
  January, 1985.
- Martti Mantyla and Reijo Sulonen, "GWB: A Solid Modeler with Euler
  Operators", IEEE Computer graphics and Applications, September 1982.
- Tony C. Woo, "A Combinatorial Analysis of Boundary Data Structure
  Schemata", IEEE Computer graphics and Applications, March 1985.
- Andrew S. Glassner, "Maintaining Winged-Edge Models", Graphics Gems II,
  James Arvo (editor), Academic Press, 1991
- Mark Segal and Carlo H. Sequin, "Partitioning Polyhedral Objects into
  Nonintersecting Parts", IEEE Computer graphics and Applications, January,
  1988.
*/

/**
A shell is a collection of connected faces representing an
orientable two-manifold. This means: a two-dimensional surface (in 3D
space here), where each point on the surface has a neighbourhood
topologically equivalent to an open disk. The surface must be orientable
and closed. Orientable means that it is two-sided. There must be a clear
distinction between its inside and its outside. The data structure
here only represent the (adjacency) topology of a surface. It is up
to the user of this data structure and library funtions to fill in
the geometry information and make sure that it is geometrically
consistent. That means that the assigned shape should be consistent with
the topology.
*/

class BREP_SOLID {
  public:
    BREP_FACE *faces;
};

#endif
