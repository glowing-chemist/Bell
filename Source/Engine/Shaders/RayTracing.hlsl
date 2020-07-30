// Struct taken from nanort.

#define epsilon 0.000000001f

#define BVH_TRAVERSAL_STACK_SIZE 32

// update these if they change cpp side.
#define VERTEX_SIZE 12
#define BVH_NODE_SIZE 40

struct BVHNode
{
  float bmin[3];
  float bmax[3];

  int flag;  // 1 = leaf node, 0 = branch node
  int axis;

  // leaf
  //   data.x = npoints
  //   data.y = index
  //
  // branch
  //   data.x = child.x
  //   data.y = child.y
  uint data[2];
};


struct TriangleIntersection 
{
  float u;
  float v;

  // Required member variables.
  float t;
  uint prim_id;
};

float2 interpolateUV(TriangleIntersection intersection, StructuredBuffer<uint> indexBuffer,  StructuredBuffer<float2> uvs)
{
	const uint baseIndiciesIndex = intersection.prim_id * 3;

	uint firstIndex = indexBuffer[baseIndiciesIndex];
	uint secondIndex = indexBuffer[baseIndiciesIndex + 1];
	uint thirdIndex = indexBuffer[baseIndiciesIndex + 2];

	float2 firstUV = uvs[firstIndex];
	float2 secondUV = uvs[secondIndex];
	float2 thirdUV = uvs[thirdIndex];

	return ((1.0f - intersection.u - intersection.v) * firstUV) + (intersection.u * secondUV) + (intersection.v * thirdUV);
}

struct MaterialOffsets
{
    uint materialIndex;
    uint materialFlags;
};


struct Ray
{
	float3 position;
	float3 direction;

	float max_t;
	float min_t;
};

float3 vsafe_inverse(in float3 v)
{
	return abs(v) < epsilon ? 1.#INF * sign(v) : 1.0f / v;
}


bool IntersectRayAABB(inout float tminOut,  // [out]
                      inout float tmaxOut,  // [out]
                      in float min_t, 
                      in float max_t,
                      in float bmin[3], 
                      in float bmax[3],
                      in float3 ray_org,
                      in float3 ray_inv_dir,
                      in int3 ray_dir_sign) 
{
  float tmin, tmax;

  const float min_x = ray_dir_sign.x ? bmax[0] : bmin[0];
  const float min_y = ray_dir_sign.y ? bmax[1] : bmin[1];
  const float min_z = ray_dir_sign.z ? bmax[2] : bmin[2];
  const float max_x = ray_dir_sign.x ? bmin[0] : bmax[0];
  const float max_y = ray_dir_sign.y ? bmin[1] : bmax[1];
  const float max_z = ray_dir_sign.z ? bmin[2] : bmax[2];

  // X
  const float tmin_x = (min_x - ray_org.x) * ray_inv_dir.x;
  // MaxMult robust BVH traversal(up to 4 ulp).
  // 1.0000000000000004 for double precision.
  const float tmax_x = (max_x - ray_org.x) * ray_inv_dir.x * 1.00000024f;

  // Y
  const float tmin_y = (min_y - ray_org.y) * ray_inv_dir.y;
  const float tmax_y = (max_y - ray_org.y) * ray_inv_dir.y * 1.00000024f;

  // Z
  const float tmin_z = (min_z - ray_org.z) * ray_inv_dir.z;
  const float tmax_z = (max_z - ray_org.z) * ray_inv_dir.z * 1.00000024f;

  tmin = max(tmin_z, max(tmin_y, max(tmin_x, min_t)));
  tmax = min(tmax_z, min(tmax_y, min(tmax_x, max_t)));

  bool result = false;

  if (tmin <= tmax) 
  {
    tminOut = tmin;
    tmaxOut = tmax;

    result = true;
  }

  return result;  // no hit
}


struct RayCoeff
{
	float Sx;
	float Sy;
	float Sz;
	int kx;
	int ky;
	int kz;
};


class TriangleIntersector
{
  // For Watertight Ray/Triangle Intersection.

  /// Do ray intersection stuff for `prim_index` th primitive and return hit
  /// distance `t`, barycentric coordinate `u` and `v`.
  /// Returns true if there's intersection.
  bool Intersect(inout float t_inout, in uint prim_index, in StructuredBuffer<uint> faces, in ByteAddressBuffer vertices)
  {
    const uint f0 = faces[3 * prim_index + 0];
    const uint f1 = faces[3 * prim_index + 1];
    const uint f2 = faces[3 * prim_index + 2];

    const float3 p0 = asfloat(vertices.Load3(f0 * VERTEX_SIZE));// vertices[f0].position.xyz;
    const float3 p1 = asfloat(vertices.Load3(f1 * VERTEX_SIZE));
    const float3 p2 = asfloat(vertices.Load3(f2 * VERTEX_SIZE));

    const float3 A = p0 - ray_org_;
    const float3 B = p1 - ray_org_;
    const float3 C = p2 - ray_org_;

    const float Ax = A[ray_coeff_.kx] - ray_coeff_.Sx * A[ray_coeff_.kz];
    const float Ay = A[ray_coeff_.ky] - ray_coeff_.Sy * A[ray_coeff_.kz];
    const float Bx = B[ray_coeff_.kx] - ray_coeff_.Sx * B[ray_coeff_.kz];
    const float By = B[ray_coeff_.ky] - ray_coeff_.Sy * B[ray_coeff_.kz];
    const float Cx = C[ray_coeff_.kx] - ray_coeff_.Sx * C[ray_coeff_.kz];
    const float Cy = C[ray_coeff_.ky] - ray_coeff_.Sy * C[ray_coeff_.kz];

    float U = Cx * By - Cy * Bx;
    float V = Ax * Cy - Ay * Cx;
    float W = Bx * Ay - By * Ax;

    // Fall back to test against edges using double precision.
    if (U == 0.0f || V == 0.0f ||
        W == 0.0f) {
      double CxBy = double(Cx) * double(By);
      double CyBx = double(Cy) * double(Bx);
      U = float(CxBy - CyBx);

      double AxCy = double(Ax) * double(Cy);
      double AyCx = double(Ay) * double(Cx);
      V = float(AxCy - AyCx);

      double BxAy = double(Bx) * double(Ay);
      double ByAx = double(By) * double(Ax);
      W = float(BxAy - ByAx);
    }

    bool result = true;
    float3 uvw = float3(U, V, W);
    result = result && all(uvw >= 0.0f);

    if (U < 0.0f || V < 0.0f ||
        W < 0.0f)
    {
      if (U > 0.0f || V > 0.0f ||
           W > 0.0f)
      {
        return false;
      }
    }

    float det = U + V + W;
    if (det == 0.0f) return false;
    result = result && det == 0.0f;

    const float Az = ray_coeff_.Sz * A[ray_coeff_.kz];
    const float Bz = ray_coeff_.Sz * B[ray_coeff_.kz];
    const float Cz = ray_coeff_.Sz * C[ray_coeff_.kz];
    const float D = U * Az + V * Bz + W * Cz;

    const float rcpDet = 1.0f / det;
    float tt = D * rcpDet;

    result = /*result &&*/ (tt < t_inout) && (tt > t_min_);

    if(result)
    {
	    t_inout = tt;
	    // Use Möller-Trumbore style barycentric coordinates
	    // U + V + W = 1.0 and interp(p) = U * p0 + V * p1 + W * p2
	    // We want interp(p) = (1 - u - v) * p0 + u * v1 + v * p2;
	    // => u = V, v = W.
	    u_ = V * rcpDet;
	    v_ = W * rcpDet;
	}

    return result;
  }

  /// Returns the nearest hit distance.
  float GetT()
   { return t_; }

  /// Update is called when initializing intersection and nearest hit is found.
  void Update(in float t, in uint prim_idx)
   {
    t_ = t;
    prim_id_ = prim_idx;
  }

  /// Prepare BVH traversal (e.g. compute inverse ray direction)
  /// This function is called only once in BVH traversal.
  void PrepareTraversal(in Ray ray)
  {
    ray_org_ = ray.position;

    // Calculate dimension where the ray direction is maximal.
    ray_coeff_.kz = 0;
    float absDir = abs(ray.direction.x);
    if (absDir < abs(ray.direction.y))
    {
      ray_coeff_.kz = 1;
      absDir = abs(ray.direction.y);
    }
    if (absDir < abs(ray.direction.z))
    {
      ray_coeff_.kz = 2;
      absDir = abs(ray.direction.z);
    }

    ray_coeff_.kx = ray_coeff_.kz + 1;
    if (ray_coeff_.kx == 3) ray_coeff_.kx = 0;
    ray_coeff_.ky = ray_coeff_.kx + 1;
    if (ray_coeff_.ky == 3) ray_coeff_.ky = 0;

    // Swap kx and ky dimension to preserve winding direction of triangles.
    if (ray.direction[ray_coeff_.kz] < 0.0f)
    {
      int tmp = ray_coeff_.kx; 
      ray_coeff_.kx = ray_coeff_.ky;
      ray_coeff_.ky = tmp; 
    }

    // Calculate shear constants.
    ray_coeff_.Sx = ray.direction[ray_coeff_.kx] / ray.direction[ray_coeff_.kz];
    ray_coeff_.Sy = ray.direction[ray_coeff_.ky] / ray.direction[ray_coeff_.kz];
    ray_coeff_.Sz = 1.0f / ray.direction[ray_coeff_.kz];

    t_min_ = ray.min_t;

    u_ = 0.0f;
    v_ = 0.0f;
  }

  /// Post BVH traversal stuff.
  /// Fill `isect` if there is a hit.
  void PostTraversal(out TriangleIntersection isect)
  {
  	isect.t = t_;
  	isect.u = u_;
  	isect.v = v_;
  	isect.prim_id = prim_id_;
  }

  float3 ray_org_;
  RayCoeff ray_coeff_;
  float t_min_;

  float t_;
  float u_;
  float v_;
  uint prim_id_;
};


bool TestLeafNode(in BVHNode node, in Ray ray, in StructuredBuffer<uint> indicies, 
				  in StructuredBuffer<uint> faces, in ByteAddressBuffer vertices,
                  inout TriangleIntersector intersector)
{
  bool hit = false;

  uint num_primitives = node.data[0];
  uint offset = node.data[1];

  float t = intersector.GetT();  // current hit distance

  float3 ray_org = ray.position;

  float3 ray_dir = ray.direction;

  for (uint i = 0; i < num_primitives; i++)
  {
    uint prim_idx = indicies[i + offset];

    float local_t = t;
    if (intersector.Intersect(local_t, prim_idx, faces, vertices)) 
    {
      // Update isect state
      t = local_t;

      intersector.Update(t, prim_idx);
      hit = true;
    }
  }

  return hit;
}


bool traverseBVH(	in StructuredBuffer<BVHNode> nodes, in StructuredBuffer<uint> indicies, // BVH buffers.
					in StructuredBuffer<uint> faces, in ByteAddressBuffer vertices,// Inndex and vertex buffers.
					in Ray ray, inout TriangleIntersector intersector,  out TriangleIntersection intersection)
{
  float hit_t = ray.max_t;

  int node_stack_index = 0;
  uint node_stack[BVH_TRAVERSAL_STACK_SIZE];
  node_stack[0] = 0;

  // Init isect info as no hit
  intersector.Update(hit_t, uint(-1));

  intersector.PrepareTraversal(ray);

  int3 dir_sign;
  dir_sign.x = ray.direction.x < 0.0f ? 1.0f : 0.0f;
  dir_sign.y = ray.direction.y < 0.0f ? 1.0f : 0.0f;
  dir_sign.z = ray.direction.z < 0.0f ? 1.0f : 0.0f;

  float3 ray_dir = ray.direction;

  float3 ray_inv_dir = vsafe_inverse(ray_dir);

  float3 ray_org = ray.position;

  float min_t = 1.#INF;
  float max_t = -1.#INF;

  while (node_stack_index >= 0)
  {
    uint index = node_stack[node_stack_index];
    BVHNode node = nodes[index];

    node_stack_index--;

    bool hit = IntersectRayAABB(min_t, max_t, ray.min_t, hit_t, node.bmin,
                                node.bmax, ray_org, ray_inv_dir, dir_sign);

    if (hit) 
    {
      // Branch node
      if (node.flag == 0)
      {
        int order_near = dir_sign[node.axis];
        int order_far = 1 - order_near;

        // Traverse near first.
        node_stack[++node_stack_index] = node.data[order_far];
        node_stack[++node_stack_index] = node.data[order_near];
      } 
      else if (TestLeafNode(node, ray, indicies, faces, vertices, intersector)) 
      {  // Leaf node
        hit_t = intersector.GetT();
      }
    }
  }

  bool hit = intersector.GetT() < ray.max_t;
  intersector.PostTraversal(intersection);

  return hit;
}
