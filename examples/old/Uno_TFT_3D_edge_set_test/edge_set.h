/* For 3D drawing, it may be handy to prepare lists of lines in advance. 
 * Ideally, each edge would be drawn once. This requires maintaining
 * some sort of set implementation. Note that there is no heap (or 
 * is there?). Tree set? Hash set? Sorted List Set? 
 * 
 * Edges for now are tuples of indecies into the vertex array. Currently
 * we are limiting models to up to 256 vertices so that we can use uint8
 * indecies. So we could use a set implementation that just stores unique
 * pairs of uint8s 
 */

/* The simplest naive set implementation for mesh edges.
 * This will happilly buffer overflow if you put too many edges in it. 
 * Also as currently configured it can store a maximum of 255 elements.
 */

#define MAX_SET_SIZE 162
 
 void insert_edge(uint8_t a, uint8_t b, uint16_t *listset, uint8_t *nelements) {
  // silently fail if the set is full
  if (*nelements==MAX_SET_SIZE) return;

  // keep edge indecies in order
  if (a>b) {uint8_t temp=b; b=a; a=temp;}
  
  // generate a key by concatenating the index for point a and point b
  uint16_t key = ((uint16_t)a<<8)|b;
  
  // binary search the sorted list
  // left and right bounds are both inclusive
  uint16_t lower = 0;
  if (*nelements!=0) {
    uint16_t upper = *nelements;
    while (lower<upper) {
      if (listset[lower]>=key) break;
      lower++;
    }
    /*
    while (lower!=upper) {
      uint16_t pivot = (upper+lower+1)>>1;
      if (listset[pivot]<=key) {
        // pivot <= key
        // key is at or to the right of pivot
        lower=pivot;
      } else {
        // pivot > key
        // key is to the left of pivot
        upper=pivot-1;
      }
    }
    */
    // Binary search converged, if we found the
    // element, don't add it, just return.
    if (listset[lower]==key) return;
  }
  
  // Insert the element in place
  // and increment the set size
  *nelements=*nelements+1;
  // Make room by shufting all elements to the right
  for (uint8_t i=*nelements; i>lower; i--) {
    listset[i] = listset[i-1];
  }
  // Insert edge
  listset[lower] = key;
}

