#include <terrain/terrain.hpp>

namespace vxe_terrain
{
    Terrain *Terrain::instance = nullptr;

    void Terrain::Update()
    {
        // worldVoxel->Update(fence, commandBuffer);
        if (observer)
        {
            glm::vec3 &playerPos = observer->transform.position;
            float invsz0 = 1.0f / ((float)(1 << 0) * 16.0f);

            int levelNum = worldVoxel->levelNum;
            std::vector<std::vector<glm::ivec4>> tempPositions;
            for (int i = 0; i < levelNum; i++)
            {
                float invsz = 1.0f / ((float)(1 << i) * 16.0f);
                centers[i] = glm::ivec4(
                    (int)glm::floor(playerPos.x * invsz),
                    (int)glm::floor(playerPos.y * invsz),
                    (int)glm::floor(playerPos.z * invsz), 0);

                std::vector<glm::ivec4> posToGenerate;
                if (centers[i] != prevCenters[i])
                {
                    // Vector3Int diff = new Vector3Int(Math.Abs());
                    glm::ivec3 minn = prevCenters[i];
                    glm::ivec3 maxx = minn + glm::ivec3(vxe_voxel::gridStorageSize, vxe_voxel::gridStorageSize, vxe_voxel::gridStorageSize);
                    for (int j = 0; j < vxe_voxel::gridStorageSize; j++)
                    {
                        for (int k = 0; k < vxe_voxel::gridStorageSize; k++)
                        {
                            for (int l = 0; l < vxe_voxel::gridStorageSize; l++)
                            {
                                glm::ivec4 pos = centers[i] + glm::ivec4(j, k, l, 0);
                                if (pos.x < minn.x || pos.x >= maxx.x ||
                                    pos.y < minn.y || pos.y >= maxx.y ||
                                    pos.z < minn.z || pos.z >= maxx.z)
                                {
                                    posToGenerate.push_back(glm::ivec4(j, k, l, 0));
                                }
                            }
                        }
                    }
                    prevCenters[i] = centers[i];
                }
                tempPositions.push_back(posToGenerate);
            }

            worldVoxel->gridCenter.ToBuffer(0, centers, worldVoxel->gridCenter.bufferSize);
            for (int i = levelNum - 1; i > 0; --i)
            {
                glm::ivec4 center = centers[i - 1];
                centers[i] = glm::ivec4(
                    (center.x & 1) == 0 ? 8 : 16,
                    (center.y & 1) == 0 ? 8 : 16,
                    (center.z & 1) == 0 ? 8 : 16, 0);
            }
            worldVoxel->gridSubMin.ToBuffer(0, centers, worldVoxel->gridCenter.bufferSize);

            for (int i = 0; i < levelNum; i++)
            {
                if (tempPositions[i].size() > 0)
                {
                    needDraw = true;
                    glm::ivec3 dim3(tempPositions[i].size(), 4, 1);
                    generator->Generate(fence, dim3, commandBuffer, i, tempPositions[i]);
                }
            }
        }

        if (needDraw)
        {
            worldVoxel->GenerateContour(fence, commandBuffer);
            needDraw = false;
        }
    }
}