#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Dicom.hpp"
#include "filesystem.hpp"

#include <dcmtk/dcmimgle/dcmimage.h>
#include <dcmtk/dcmdata/dctk.h>
#include <vector>

using namespace std;

struct Slice {
    DicomImage* image;
    double3 spacing;
    double location;
};

Slice ReadDicomSlice(const string& file) {
    DcmFileFormat fileFormat;
    fileFormat.loadFile(file.c_str());
    DcmDataset* dataset = fileFormat.getDataset();

    double3 s = 0;
    dataset->findAndGetFloat64(DCM_PixelSpacing, s.x, 0);
    dataset->findAndGetFloat64(DCM_PixelSpacing, s.y, 1);
    dataset->findAndGetFloat64(DCM_SliceThickness, s.z, 0);

    double x = 0;
    dataset->findAndGetFloat64(DCM_SliceLocation, x, 0);

    return { new DicomImage(file.c_str()), s, x };
}

Dicom::Dicom() {
    volume.data = nullptr;
    volume.width = 0;
    volume.height = 0;
    volume.depth = 0;
    volume.size = 0;
}

Dicom::~Dicom() {
    delete[] volume.data;
}

int Dicom::LoadDicomStack(const string& folder, float3* size) {
    if (!filesystem::exists(folder)) {
        printf("Folder does not exist\n");
        return -1;
    }

    double3 maxSpacing = 0;
    vector<Slice> images = {};
    for (const auto& p : filesystem::directory_iterator(folder))
        if (p.path().extension().string() == ".dcm") {
            images.push_back(ReadDicomSlice(p.path().string()));
            if (images[images.size() - 1].spacing.x > maxSpacing.x &&
                    images[images.size() - 1].spacing.y > maxSpacing.y) {
                maxSpacing = images[images.size() - 1].spacing;
            }
        }

    if (images.empty()) return -1;

    std::sort(images.begin(), images.end(), [](const Slice& a, const Slice& b) {
            return a.location < b.location;
            });

    volume.width = images[0].image->getWidth();
    volume.height = images[0].image->getHeight();
    volume.depth = (uint32_t)images.size();

    if (volume.width == 0 || volume.height == 0) return -1;

    // volume size in meters
    if (size) {
        double2 b = images[0].location;
        for (auto i : images) {
            b.x = (float)fmin(i.location - i.spacing.z * .5, b.x);
            b.y = (float)fmax(i.location + i.spacing.z * .5, b.y);
        }

        *size = float3(.001 * double3(maxSpacing.xy * double2(volume.width, volume.height), b.y - b.x));
        volume.size = *size;
        printf("%fm x %fm x %fm\n", size->x, size->y, size->z);
    }

    if (volume.data != nullptr) {
        delete[] volume.data;
    }

    bool organ_masks = filesystem::exists(std::filesystem::path(folder) / "mask");
    if (organ_masks) {
        cout << "Applying organ masks" << endl;
    }

    volume.data = new uint16_t[volume.width * volume.height * volume.depth];
    memset(volume.data, 0, volume.width * volume.height * volume.depth * sizeof(uint16_t));
    for (uint32_t i = 0; i < images.size(); i++) {
        images[i].image->setMinMaxWindow();
        uint16_t* pixels = (uint16_t*)images[i].image->getOutputData(16);
        memcpy(volume.data + i * volume.width * volume.height, pixels, volume.width * volume.height * sizeof(uint16_t));

        if (organ_masks) {
            std::filesystem::path path(folder);
            path = path / "mask" / (std::to_string(i) + ".png");

            int x, y, n;
            stbi_set_flip_vertically_on_load(true);
            uint8_t* data = stbi_load(path.c_str(), &x, &y, &n, 1);
            if (data == nullptr) {
                cerr << "Failed to load mask " << path << endl;
                cerr << stbi_failure_reason() << endl;
                return -1;
            }

            if (x != volume.width || y != volume.height) {
                cerr << "FATAL: " << i << ".png has an unexpected size/format" << endl;
                cerr << "x: " << x << " y: " << y << " n: " << n << endl;
                return -1;
            }

            for (uint16_t mask_x = 0; mask_x < volume.width; mask_x++) {
                for (uint16_t mask_y = 0; mask_y < volume.height; mask_y++) {
                    if (data[mask_x + (mask_y * volume.width)] == 0) {
                        //volume.data[mask_x + volume.width * (mask_y + volume.depth * i)] = 0;
                    }
                }
            }
        }
    }

    if (organ_masks) {
        cout << "Zeroed out all non-masked samples" << endl;
    }

    for (auto& i : images) delete i.image;

    return 0;
}
