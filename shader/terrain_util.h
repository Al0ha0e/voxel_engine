#ifndef TERRAIN_UTIL_H
#define TERRAIN_UTIL_H

#define gridStorageSize 4
#define gridRenderSize 3
#define blockSize 16
#define gridStorageSize3 (gridStorageSize * gridStorageSize * gridStorageSize)
#define blockSize3 (blockSize * blockSize * blockSize)
// #define levelContourSize (gridStorageSize3 * blockSize3 * 3 * 4 * 3)
#define levelContourSize (gridStorageSize3 * blockSize3 * 3 * 3 * 3)

uint GetEdgeIdx(uint level, uint dim, uvec3 block, uvec3 cell, uvec3 offset)
{
    block = (block + offset) & 3;
    return (((level * gridStorageSize3 + ((block.z * gridStorageSize + block.y) * gridStorageSize + block.x)) * 3 + dim) << 12) + (((cell.z << 4) + cell.y) << 4) + cell.x;
}

uint GetVertIdx(uint level, uvec3 block, uvec3 cell, uvec3 offset)
{
    block = (block + offset) & 3;
    return ((level * gridStorageSize3 + ((block.z * gridStorageSize + block.y) * gridStorageSize + block.x)) << 12) + (((cell.z << 4) + cell.y) << 4) + cell.x;
}

uint GetEdgeIdx(uint level, uint dim, uvec3 global_cell, uvec3 offset)
{
    uvec3 block = global_cell >> 4;
    block = (block + offset) & 3;
    uvec3 cell = global_cell & 15;
    return (((level * gridStorageSize3 + ((block.z * gridStorageSize + block.y) * gridStorageSize + block.x)) * 3 + dim) << 12) + (((cell.z << 4) + cell.y) << 4) + cell.x;
}

uint GetVertIdx(uint level, uvec3 global_cell, uvec3 offset)
{
    uvec3 block = global_cell >> 4;
    block = (block + offset) & 3;
    uvec3 cell = global_cell & 15;
    return ((level * gridStorageSize3 + ((block.z * gridStorageSize + block.y) * gridStorageSize + block.x)) << 12) + (((cell.z << 4) + cell.y) << 4) + cell.x;
}

uvec3 GetPosFromEIdx(uint eidx, uint level, uint dim, uvec3 offset)
{
    eidx -= (level * gridStorageSize3 * 3 + dim) << 12;
    uvec3 block_pos, cell_pos;
    uint bsz = 3 << 12;
    uint bsz1d = gridStorageSize * (3 << 12);
    uint bsz2d = bsz1d * gridStorageSize;
    block_pos.z = eidx / bsz2d;
    eidx -= block_pos.z * bsz2d;
    block_pos.y = eidx / bsz1d;
    eidx -= block_pos.y * bsz1d;
    block_pos.x = eidx / bsz;
    eidx -= block_pos.x * bsz;

    cell_pos.z = eidx >> 8;
    eidx &= 255;
    cell_pos.y = eidx >> 4;
    eidx &= 15;
    cell_pos.x = eidx;

    block_pos = (block_pos + (uvec3(4, 4, 4) - offset)) & 3;

    return (block_pos << 4) + cell_pos;
}

uvec3 GetPosFromVIdx(uint vidx, uint level, uvec3 offset)
{
    vidx -= (level * gridStorageSize3) << 12;
    uvec3 block_pos, cell_pos;
    uint bsz1d = gridStorageSize << 12;
    uint bsz2d = bsz1d * gridStorageSize;
    block_pos.z = vidx / bsz2d;
    vidx -= block_pos.z * bsz2d;
    block_pos.y = vidx / bsz1d;
    vidx -= block_pos.y * bsz1d;
    block_pos.x = vidx >> 12;
    vidx &= (1 << 12) - 1;

    cell_pos.z = vidx >> 8;
    vidx &= 255;
    cell_pos.y = vidx >> 4;
    vidx &= 15;
    cell_pos.x = vidx;

    block_pos = (block_pos + (uvec3(4, 4, 4) - offset)) & 3;

    return (block_pos << 4) + cell_pos;
}

ivec3 GetCellWorldPosition(ivec3 global_cell_pos, ivec3 grid_center, uint level)
{
    global_cell_pos += (grid_center << 4) - blockSize;
    return global_cell_pos << level;
}

#endif