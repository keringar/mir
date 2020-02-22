#include "Dicom.hpp"

#include <dcmtk/dcmimgle/dcmimage.h>
#include <dcmtk/dcmdata/dctk.h>
#include <vector>
#include <experimental/filesystem>

using namespace std;
using namespace std::experimental::filesystem;

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
    data = nullptr;
    width = 0;
    height = 0;
    depth = 0;
}

Dicom::~Dicom() {
    delete[] data;
}

int Dicom::LoadDicomStack(const string& folder, float3* size) {
    if (!exists(folder)) {
        printf("Folder does not exist\n");
        return -1;
    }

    double3 maxSpacing = 0;
    vector<Slice> images = {};
    for (const auto& p : directory_iterator(folder))
        if (p.path().extension().string() == ".dcm") {
            images.push_back(ReadDicomSlice(p.path().string()));
            maxSpacing = max(maxSpacing, images[images.size() - 1].spacing);
        }

    if (images.empty()) return -1;

    std::sort(images.begin(), images.end(), [](const Slice& a, const Slice& b) {
            return a.location < b.location;
            });

    width = images[0].image->getWidth();
    height = images[0].image->getHeight();
    depth = (uint32_t)images.size();

    if (width == 0 || height == 0) return -1;

    // volume size in meters
    if (size) {
        float2 b = images[0].location;
        for (auto i : images) {
            b.x = (float)fmin(i.location - i.spacing.z * .5, b.x);
            b.y = (float)fmax(i.location + i.spacing.z * .5, b.y);
        }

        *size = float3(.001 * double3(maxSpacing.xy * double2(width, height), b.y - b.x));
        printf("%fm x %fm x %fm\n", size->x, size->y, size->z);
    }

    if (data != nullptr) {
        delete[] data;
    }

    data = new uint16_t[width * height * depth];
    memset(data, 0, width * height * depth * sizeof(uint16_t));
    for (uint32_t i = 0; i < images.size(); i++) {
        images[i].image->setMinMaxWindow();
        uint16_t* pixels = (uint16_t*)images[i].image->getOutputData(16);
        memcpy(data + i * width * height, pixels, width * height * sizeof(uint16_t));
    }

    for (auto& i : images) delete i.image;

    return 0;
}
