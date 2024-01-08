#ifndef QEF_H
#define QEF_H

#define SVD_NUM_SWEEPS 5
#define PSUEDO_INVERSE_THRESHOLD (0.1f)

void QEFAdd(
    vec3 n,
    vec3 p,
    inout mat3 ATA,
    inout vec4 ATb,
    inout vec4 pointaccum)
{
    // ATA._m00_m01_m02 += n.x * n;
    vec3 tmp = n.x * n;
    ATA[0][0] += tmp.x;
    ATA[1][0] += tmp.y;
    ATA[2][0] += tmp.z;
    // ATA._m11_m12 += n.y * n.yz;
    tmp.yz = n.y * n.yz;
    ATA[1][1] += tmp.y;
    ATA[2][1] += tmp.z;
    // ATA._m22 += n.z * n.z;
    tmp.z = n.z * n.z;
    ATA[2][2] += tmp.z;

    float b = dot(p, n);
    ATb.xyz += n * b;

    pointaccum.xyz += p;
    pointaccum.w += 1.0f;
}

void givens_coeffs_sym(float a_pp, float a_pq, float a_qq, inout float c, inout float s)
{
    if (a_pq == 0.f)
    {
        c = 1.f;
        s = 0.f;
        return;
    }
    float tau = (a_qq - a_pp) / (2.f * a_pq);
    float stt = sqrt(1.f + tau * tau);
    float t = 1.f / ((tau >= 0.f) ? (tau + stt) : (tau - stt));
    c = inversesqrt(1.f + t * t);
    s = t * c;
}

void svd_rotate_xy(inout float x, inout float y, float c, float s)
{
    float u = x;
    float v = y;
    x = c * u - s * v;
    y = s * u + c * v;
}

void svd_rotateq_xy(inout float x, inout float y, inout float a, float c, float s)
{
    float cc = c * c;
    float ss = s * s;
    float mx = 2.0 * c * s * a;
    float u = x;
    float v = y;
    x = cc * u - mx + ss * v;
    y = ss * u + mx + cc * v;
}

void svd_rotate(inout mat3 vtav, inout mat3 v, int a, int b)
{
    if (vtav[b][a] == 0.0)
        return;

    float c, s;
    givens_coeffs_sym(vtav[a][a], vtav[b][a], vtav[b][b], c, s);

    float x, y, z;
    x = vtav[a][a];
    y = vtav[b][b];
    z = vtav[b][a];
    svd_rotateq_xy(x, y, z, c, s);
    vtav[a][a] = x;
    vtav[b][b] = y;
    vtav[b][a] = z;

    x = vtav[3 - b][0];
    y = vtav[2][1 - a];
    svd_rotate_xy(x, y, c, s);
    vtav[3 - b][0] = x;
    vtav[2][1 - a] = y;

    vtav[b][a] = 0.0;

    x = v[a][0];
    y = v[b][0];
    svd_rotate_xy(x, y, c, s);
    v[a][0] = x;
    v[b][0] = y;

    x = v[a][1];
    y = v[b][1];
    svd_rotate_xy(x, y, c, s);
    v[a][1] = x;
    v[b][1] = y;

    x = v[a][2];
    y = v[b][2];
    svd_rotate_xy(x, y, c, s);
    v[a][2] = x;
    v[b][2] = y;
}

void svd_solve_sym(mat3 a, out vec4 sigma, inout mat3 v)
{
    // assuming that A is symmetric: can optimize all operations for
    // the upper right triagonal
    mat3 vtav = a;
    // vtav[0][0] = a[0][0]; vtav[0][1] = a[0][1]; vtav[0][2] = a[0][2];
    // vtav[1][0] = 0.f;  vtav[1][1] = a[3]; vtav[1][2] = a[4];
    // vtav[2][0] = 0.f;  vtav[2][1] = 0.f;  vtav[2][2] = a[5];

    // assuming V is identity: you can also pass a matrix the rotations
    // should be applied to. (U is not computed)
    for (int i = 0; i < SVD_NUM_SWEEPS; ++i)
    {
        svd_rotate(vtav, v, 0, 1);
        svd_rotate(vtav, v, 0, 2);
        svd_rotate(vtav, v, 1, 2);
    }

    sigma = vec4(vtav[0][0], vtav[1][1], vtav[2][2], 0.f);
}

float svd_invdet(float x, float tol)
{
    return (abs(x) < tol || abs(1.0 / x) < tol) ? 0.0 : (1.0 / x);
}

void svd_pseudoinverse(out mat3 o, vec4 sigma, mat3 v)
{
    float d0 = svd_invdet(sigma.x, PSUEDO_INVERSE_THRESHOLD);
    float d1 = svd_invdet(sigma.y, PSUEDO_INVERSE_THRESHOLD);
    float d2 = svd_invdet(sigma.z, PSUEDO_INVERSE_THRESHOLD);

    o[0][0] = v[0][0] * d0 * v[0][0] + v[1][0] * d1 * v[1][0] + v[2][0] * d2 * v[2][0];
    o[1][0] = v[0][0] * d0 * v[0][1] + v[1][0] * d1 * v[1][1] + v[2][0] * d2 * v[2][1];
    o[2][0] = v[0][0] * d0 * v[0][2] + v[1][0] * d1 * v[1][2] + v[2][0] * d2 * v[2][2];

    o[0][1] = v[0][1] * d0 * v[0][0] + v[1][1] * d1 * v[1][0] + v[2][1] * d2 * v[2][0];
    o[1][1] = v[0][1] * d0 * v[0][1] + v[1][1] * d1 * v[1][1] + v[2][1] * d2 * v[2][1];
    o[2][1] = v[0][1] * d0 * v[0][2] + v[1][1] * d1 * v[1][2] + v[2][1] * d2 * v[2][2];
    o[0][2] = v[0][2] * d0 * v[0][0] + v[1][2] * d1 * v[1][0] + v[2][2] * d2 * v[2][0];
    o[1][2] = v[0][2] * d0 * v[0][1] + v[1][2] * d1 * v[1][1] + v[2][2] * d2 * v[2][1];
    o[2][2] = v[0][2] * d0 * v[0][2] + v[1][2] * d1 * v[1][2] + v[2][2] * d2 * v[2][2];
}

void svd_mul_matrix_vec(out vec4 result, mat3 a, vec4 b)
{
    result.x = dot(vec4(a[0][0], a[1][0], a[2][0], 0.f), b);
    result.y = dot(vec4(a[0][1], a[1][1], a[2][1], 0.f), b);
    result.z = dot(vec4(a[0][2], a[1][2], a[2][2], 0.f), b);
    result.w = 0.f;
}

void SVDSolveATAATb(
    mat3 ATA,
    vec4 ATb,
    out vec4 x)
{
    mat3 V;
    V[0][0] = 1.f;
    V[1][0] = 0.f;
    V[2][0] = 0.f;
    V[0][1] = 0.f;
    V[1][1] = 1.f;
    V[2][1] = 0.f;
    V[0][2] = 0.f;
    V[1][2] = 0.f;
    V[2][2] = 1.f;

    vec4 sigma = {0.f, 0.f, 0.f, 0.f};
    svd_solve_sym(ATA, sigma, V);

    // A = UEV^T; U = A / (E*V^T)
    mat3 Vinv;
    svd_pseudoinverse(Vinv, sigma, V);
    svd_mul_matrix_vec(x, Vinv, ATb);
}

void SVDVmulSym(out vec4 result, mat3 A, vec4 v)
{
    vec4 A_row_x = {A[0][0], A[1][0], A[2][0], 0.0f};

    result.x = dot(A_row_x, v);
    result.y = A[1][0] * v.x + A[1][1] * v.y + A[2][1] * v.z;
    result.z = A[2][0] * v.x + A[2][1] * v.y + A[2][2] * v.z;
    result.w = 0;
}

float QEFCalcError(mat3 A, vec4 x, vec4 b)
{
    vec4 tmp;

    SVDVmulSym(tmp, A, x);
    tmp = b - tmp;

    return dot(tmp, tmp);
}

float QEFSolve(
    mat3 ATA,
    vec4 ATb,
    vec4 pointaccum,
    out vec4 x)
{
    vec4 masspoint = pointaccum / pointaccum.w;

    vec4 A_mp = {0.f, 0.f, 0.f, 0.f};
    SVDVmulSym(A_mp, ATA, masspoint);
    A_mp = ATb - A_mp;

    SVDSolveATAATb(ATA, A_mp, x);

    float error = QEFCalcError(ATA, x, ATb);
    // x += masspoint;
    x = masspoint;

    return error;
}

#endif