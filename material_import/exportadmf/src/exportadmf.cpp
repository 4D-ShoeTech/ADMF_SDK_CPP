//
//  exportadmf.c
//  exportadmf
//
//  Created by yushihang on 2021/7/1.
//

#include <stdio.h>
#include <string>
#include <iostream>

#include <chrono>
#include <sstream>
#include <fstream>
#if (defined _WIN32) || (defined __APPLE__)
#include <filesystem>
#else
#include <iomanip> 
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <unistd.h>
#endif
#include "FreeImage/FreeImage.h"
#include "changecolor/changecolor.h"
#include "admf_cpp.h"
#include "admf_internal_header.h"

#if (!defined __APPLE__)
#include <assert.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
    
    void exportChangeColor(const std::string& path, const CHANGE_COLOR::Result& result)
    {
        try {
            std::ofstream out(path);
            out << "{" << std::endl;
            out << "\t\"hasDiffuseMap\": true," << std::endl;
            out << "\t\"bottomS\": " << std::setprecision(20) << result.bottomS << "," << std::endl;
            out << "\t\"bottomV\": " << std::setprecision(20) << result.bottomV << "," << std::endl;
            out << "\t\"meanS\": " << std::setprecision(20) << result.meanS << "," << std::endl;
            out << "\t\"meanV\": " << std::setprecision(20) << result.meanV << "," << std::endl;
            out << "\t\"kS\": " << std::setprecision(20) << result.kS << "," << std::endl;
            out << "\t\"kV\": " << std::setprecision(20) << result.kV  << std::endl;
            out << "}" << std::endl;
            out.close();
            
            
            
            
        }
        catch (...) {
            
        }
    }
    
    bool ExportImageDataToFile(const unsigned char* buffer, const std::string& destPath, int width, int height, int channel, const int elementSize) {
        FREE_IMAGE_TYPE imageType = FIT_UNKNOWN;
        
        switch (channel)
        {
        case 1:
            imageType = FIT_BITMAP;
            break;
        case 2:
            imageType = FIT_BITMAP;
            break;
        case 3:
            if (elementSize == 1) {
                imageType = FIT_BITMAP;
            }
            else if (elementSize == 2) {
                imageType = FIT_RGB16;
            }
            else if (elementSize == 4) {
                imageType = FIT_RGBF;
            }
            break;
        case 4:
            if (elementSize == 1) {
                imageType = FIT_BITMAP;
            }
            else if (elementSize == 2) {
                imageType = FIT_RGBA16;
            }
            else if (elementSize == 4) {
                imageType = FIT_RGBAF;
            }
            break;
        default:
            break;
        }
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
  
        FIBITMAP* bitmap = FreeImage_ConvertFromRawBitsEx(true, (BYTE*)buffer, imageType,
                                                          width, height, width * channel * elementSize,
                                                          channel * elementSize * 8, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK,
                                                          true);
        
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span = t2-t1;
        printf("FreeImage_ConvertFromRawBitsEx:%f\n", time_span.count());
        if (!bitmap) {
            return false;
        }
        
        //FreeImage_FlipHorizontal(bitmap);
        FreeImage_FlipVertical(bitmap);
        std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();
         time_span = t3-t2;
        printf("FreeImage_FlipVertical:%f\n", time_span.count());
        auto info = FreeImage_GetInfo(bitmap);
        std::chrono::high_resolution_clock::time_point t4 = std::chrono::high_resolution_clock::now();
        time_span = t4-t3;
        printf("FreeImage_GetInfo:%f\n", time_span.count());

        bool bSuccess = FreeImage_Save(FIF_PNG, bitmap, destPath.c_str(), PNG_DEFAULT);
        std::chrono::high_resolution_clock::time_point t5 = std::chrono::high_resolution_clock::now();
        time_span = t5-t4;
        printf("FreeImage_Save:%f\n", time_span.count());
        FreeImage_Unload(bitmap);
        std::chrono::high_resolution_clock::time_point t6 = std::chrono::high_resolution_clock::now();
        time_span = t6-t5;
        printf("FreeImage_Unload:%f\n", time_span.count());
        return true;
    }
    
    bool hasSuffix(std::string const &fullString, std::string const &suffix) {
        if (fullString.length() >= suffix.length()) {
            return (0 == fullString.compare (fullString.length() - suffix.length(), suffix.length(), suffix));
        } else {
            return false;
        }
    }
    
    void extractLayer(const std::string& pathName,  const admf::MaterialLayer& layer, const std::string& layerIndex)
    {
        if (!layer->isEnabled())
            return;
        
        auto basic = layer->getBasic();
        
        std::vector<admf::Texture> textureVector;
        
        auto vectorLambda = [&textureVector](const admf::Texture& texture) {
            if (texture)
            {
                auto binary = texture->getBinaryData();
                if (binary)
                {
                    auto len = binary->getDataLength();
                    if (len > 0)
                    {
                        textureVector.push_back(texture);
                        return true;
                    }
                }
                
            }
            return false;
        };
        auto needExportDiffuse = vectorLambda(basic->getBaseColor()->getTexture());
        //这句必须在最前
        vectorLambda(basic->getAlpha()->getTexture());
        vectorLambda(basic->getNormal()->getTexture());
        vectorLambda(basic->getSpecular()->getTexture());
        vectorLambda(basic->getMetalness()->getTexture());
        vectorLambda(basic->getRoughness()->getTexture());
        vectorLambda(basic->getGlossiness()->getTexture());
        vectorLambda(basic->getAnisotropy()->getTexture());
        vectorLambda(basic->getAnisotropyRotation()->getTexture());
        vectorLambda(basic->getEmissive()->getTexture());
        for (auto& texture : textureVector)
        {
            auto binaryData = texture->getBinaryData();
            if (!binaryData)
                continue;
            
            admf_internal::BinaryData_internal* binaryData_internal = dynamic_cast<admf_internal::BinaryData_internal*>(binaryData.get());
            
            auto name = binaryData_internal->getRawName();
            char* nameBuff = new char[name->getLength()];
            name->getString(nameBuff, name->getLength());
            auto texturePath = pathName + "/" + nameBuff;
            
            
            admf_internal::Texture_internal* texture_internal = dynamic_cast<admf_internal::Texture_internal*>(texture.get());
            auto textureBinaryType = texture_internal->getTypeByBinaryData();
            auto ext = texture_internal->getExtensionByTextureFileType(textureBinaryType);
            
            assert(hasSuffix(texturePath, ext));
            
            
            auto dataLen = binaryData->getDataLength();
            unsigned char* dataBuff = (unsigned char*)malloc(dataLen);
            binaryData->getData(dataBuff, dataLen);
            
            if (needExportDiffuse)
            {
              
                CHANGE_COLOR::Result result;
               
                if (textureBinaryType == admf::TextureFileType::RAW)
                    result = CHANGE_COLOR::changeColor(dataBuff, texture->getWidth(), texture->getHeight(), texture->getChannels());
                else
                {
                    FIMEMORY* stream = FreeImage_OpenMemory();
                    FreeImage_WriteMemory(dataBuff, 1, dataLen, stream);
                    FreeImage_SeekMemory(stream, 0, SEEK_SET);
                    
                    FREE_IMAGE_FORMAT format = FIF_PNG;
                    switch (textureBinaryType) {
                    case admf::TextureFileType::PNG:
                        format = FIF_PNG;
                        break;
                    case admf::TextureFileType::JPG:
                        format = FIF_JPEG;
                        break;
                    case admf::TextureFileType::GIF:
                        format = FIF_GIF;
                        break;
                    case admf::TextureFileType::TIFF:
                        format = FIF_TIFF;
                        break;
                        
                    default:
                        break;
                    }
                    FIBITMAP* bitmap = FreeImage_LoadFromMemory(FIF_PNG, stream);
                    
              
                    int width = FreeImage_GetWidth(bitmap);
                    int height = FreeImage_GetHeight(bitmap);
                    int scan_width = FreeImage_GetPitch(bitmap);
                    int bpp = FreeImage_GetBPP(bitmap);
                    BYTE *bits = (BYTE*)malloc(height * scan_width);
                    // convert the bitmap to raw bits (top-left pixel first)
                    FreeImage_ConvertToRawBits(bits, bitmap, scan_width, bpp, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, FALSE);
                    FreeImage_Unload(bitmap);
                  
                    FreeImage_CloseMemory(stream);
                    
                    result = CHANGE_COLOR::changeColor(bits, width, height, bpp / 8);
                    
                    free(bits);
                }
                
                needExportDiffuse = false;
                exportChangeColor(pathName + "/changeColor" + layerIndex + ".json", result);
            }
            std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
            if (textureBinaryType != admf::TextureFileType::RAW)
            {
                auto myfile = std::fstream(texturePath, std::ios::out | std::ios::binary);
                myfile.write((char*)dataBuff, dataLen);
                myfile.close();
            }
            else
            {
                ExportImageDataToFile((unsigned char*)dataBuff, texturePath, texture->getWidth(), texture->getHeight(), texture->getChannels(), texture->getElementSize());
            }
            
            std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> time_span = t2-t1;
            printf("ExportImageDataToFile:%f\n", time_span.count());
 
            free(dataBuff);
            dataBuff = nullptr;
            //binaryData->exportToFile(texturePath.c_str());
            delete[] nameBuff;
        }
    }
    
    
    
    bool extractAdmf(const char* admfFilePath, const char* dir_)
    {
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        printf("\nexport %s to dir %s\n", admfFilePath, dir_);
        if (admfFilePath == nullptr || dir_ == nullptr || strlen(admfFilePath) == 0 || strlen(dir_) == 0)
            return false;
        std::string dir(dir_);
#if (defined __APPLE__) || (defined _WIN32)
#ifdef __APPLE__
        namespace fs = std::__fs::filesystem;
#else
        namespace fs = std::filesystem;
#endif
        if (!fs::exists(dir))
        {
            printf("dir %s not exist\n", dir_);
            return false;
        }
        
#else
        struct stat st;
        if(stat(dir_,&st) == 0)
        {
            if(((st.st_mode) & S_IFMT) != S_IFDIR)
                return false;
        }
        else
            return false;
#endif
        FreeImage_Initialise();
        if (dir[dir.length()-1] == '\\' || dir[dir.length()-1] == '/')
            dir.pop_back();
        
        admf::ADMF_RESULT result;
        admf_internal::ADMF_internal::ADMFJsons admfJsons;
        auto admf = admf_internal::ADMF_internal::loadFromFile(admfFilePath, result, &admfJsons);
        if (!admf)
        {
            printf("admf %s open failed\n", admfFilePath);
            return false;
        }
        
        
        {
            std::ofstream jsonFile(dir + "/material.json");
            jsonFile << admfJsons.material;
        }
        {
            std::ofstream jsonFile(dir + "/custom.json");
            jsonFile << admfJsons.custom;
        }
        {
            std::ofstream jsonFile(dir + "/geometry.json");
            jsonFile << admfJsons.geometry;
        }
        {
            std::ofstream jsonFile(dir + "/physics.json");
            jsonFile << admfJsons.physics;
        }
        std::string layersPath = dir;
        /*
         std::string layersPath = dir + "/textures";
         #ifdef _WIN32
         _mkdir(layersPath.c_str());
         #else
         mkdir(layersPath.c_str(), 0777);
         #endif
         */
        
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span = t2-t1;
        printf("before extractLayer:%f\n", time_span.count());
        auto material = admf->getMaterial();
        auto layerArray = material->getLayerArray();
        auto layersCount = layerArray->size();
        for (int i = 0; i < layersCount; i++) {
            auto layer = layerArray->get(i);
            extractLayer(layersPath, layer, i==0 ? "":std::to_string(i).c_str());
        }
        
        auto sideLayer = material->getSideLayer();
        extractLayer(layersPath,  sideLayer, "Side");
        std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();
         time_span = t3-t2;
        printf("after extractLayer:%f\n", time_span.count());
        
        //admf->getMaterial()->getMetaData()->getSource()->exportToFile((dir + "/orig.4ddat").c_str());
        FreeImage_DeInitialise();
        printf("\nexport %s to dir %s finished\n", admfFilePath, dir_);
        return true;
    }
    
#ifdef __cplusplus
}
#endif
