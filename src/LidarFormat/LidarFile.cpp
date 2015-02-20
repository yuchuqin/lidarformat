/***********************************************************************

This file is part of the LidarFormat project source files.

LidarFormat is an open source library for efficiently handling 3D point 
clouds with a variable number of attributes at runtime. 


Homepage: 

    http://code.google.com/p/lidarformat

Copyright:

    Institut Geographique National & CEMAGREF (2009)

Author: 

    Adrien Chauve

Contributors:

    Nicolas David, Olivier Tournaire



    LidarFormat is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LidarFormat is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with LidarFormat.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************/



#include <stdexcept>
#include <iostream>
#include <sstream>

#include "boost/filesystem.hpp"
#include <boost/shared_ptr.hpp>

#include "LidarDataContainer.h"
#include "LidarIOFactory.h"
#include "LidarFormat/geometry/LidarCenteringTransfo.h"

#include "LidarFormat/LidarFile.h"
#include "file_formats/PlyArchi/Ply2Lf.h"
#ifdef ENABLE_LAS
#include "file_formats/LAS/Las2Lf.h"
#endif // ENABLE_LAS

using namespace boost::filesystem;

namespace Lidar
{

std::string LidarFile::getMetaData() const
{
    if(!isValid())
        throw std::logic_error("Error : Lidar xml file is not valid !\n");

    using namespace std;
    ostringstream result;
    result << "Nb of points : " << m_xmlData->attributes().dataSize() << "\n";
    result << "Format : " << m_xmlData->attributes().dataFormat() << "\n";
    result << "Binary filename : " << getBinaryDataFileName() << "\n";
    return result.str();
}

std::string LidarFile::getFormat() const
{
    if(!isValid())
        throw std::logic_error("Error : Lidar xml file is not valid !\n");

    return m_xmlData->attributes().dataFormat();
}

std::string LidarFile::getBinaryDataFileName() const
{
    if(!isValid())
        throw std::logic_error("Error : Lidar xml file is not valid !\n");

    path fileName = path(m_xmlFileName).branch_path();

    if(m_xmlData->attributes().dataFileName().present())
    {
        path datafilename(std::string(m_xmlData->attributes().dataFileName().get()));
        // BV: check if path is absolute or relative
        if(datafilename.is_absolute()) fileName = datafilename;
        else fileName /= datafilename;
    }
    else
    {
        fileName /= (basename(m_xmlFileName) + ".bin");
    }

    return fileName.string();
}

unsigned int LidarFile::getNbPoints() const
{
    if(!isValid())
        throw std::logic_error("Error : Lidar xml file is not valid !\n");

    return (unsigned int)m_xmlData->attributes().dataSize();
}

LidarFile::LidarFile(const std::string &filename):
    m_isValid(false)
{
    // BV: we want to tolerate loading directly other formats =>
    // if ext is not .xml, do the following:
    // if there is a .xml next to it, use it
    // if not, generate it (in both cases dataFileName is relative)
    // if it cannot be generated (read only) generate it in cwd => dataFileName must be absolute
    path filepath(filename);
    if(filepath.extension().string() != ".xml") // filename is not xml, we need one
    {
        path xml_filepath(filename);
        xml_filepath.replace_extension(".xml");
        if(exists(xml_filepath)) // we found an xml, use it
        {
            m_xmlFileName = xml_filepath.string();
            std::cout << "Using existing xml file " << m_xmlFileName << std::endl;
        }
        else // no xml found, create one
        {
            if(filepath.extension().string() == ".ply")  m_xmlFileName = WritePlyXmlHeader(filename);
#ifdef ENABLE_LAS
            if(filepath.extension().string() == ".las")  m_xmlFileName = WriteLasXmlHeader(filename);
#endif // ENABLE_LAS
            m_isValid = !m_xmlFileName.empty();
            if(!m_isValid) std::cout << "Could not generate an xml file" << std::endl;
        }
    }
    else m_xmlFileName = filename; // filename is xml, use it

    // load the xml (either given as argument or generated)
    try
    {
        std::auto_ptr<cs::LidarDataType> ap(cs::lidarData(m_xmlFileName, xml_schema::Flags::dont_validate));
        m_xmlData  = boost::shared_ptr<cs::LidarDataType>(ap);
        m_isValid =  true;
    }

    catch( const std::exception &)
    {
        m_isValid =  false;
    }

}

void LidarFile::loadData(LidarDataContainer& lidarContainer)
{
    if(!isValid())
        throw std::logic_error("Error : Lidar xml file is not valid !\n");

    // create reader using factory
    boost::shared_ptr<LidarFileIO> reader = LidarIOFactory::instance().createObject(getFormat());

    loadMetaDataFromXML();
    lidarContainer.setMapsFromXML(m_xmlData);
    lidarContainer.resize(m_lidarMetaData.nbPoints_);

    reader->setXMLData(m_xmlData);
    reader->loadData(lidarContainer, m_lidarMetaData, m_attributeMetaData);

}

void LidarFile::loadTransfo(LidarCenteringTransfo& transfo) const
{
    transfo.setTransfo(0,0);

    if(!isValid())
        return;

    if(!m_xmlData->attributes().centeringTransfo().present())
        return;

    cs::CenteringTransfoType transfoXML = m_xmlData->attributes().centeringTransfo().get();
    transfo.setTransfo(transfoXML.tx(), transfoXML.ty());

}

shared_ptr<cs::LidarDataType> LidarFile::createXMLStructure(
        const LidarDataContainer& lidarContainer,
        const std::string& dataFileName,
        const LidarCenteringTransfo& transfo,
        const cs::DataFormatType format)
{
    // generate xml file
    cs::LidarDataType::AttributesType attributes(lidarContainer.size(), format);

    // insert attributes in the xml (with their names and types)
    const AttributeMapType& attributeMap = lidarContainer.getAttributeMap();
    for(AttributeMapType::const_iterator it=attributeMap.begin(); it!=attributeMap.end(); ++it)
    {
        attributes.attribute().push_back(it->second);
    }

    // add transfo
    if(transfo.isSet())
    {
        attributes.centeringTransfo(cs::CenteringTransfoType(transfo.x(), transfo.y()));
    }

    // add dataFilename
    attributes.dataFileName() = dataFileName;

    shared_ptr<cs::LidarDataType> xmlStructure(new cs::LidarDataType(attributes));
    return xmlStructure;
}

void LidarFile::save(const LidarDataContainer& lidarContainer,
                     const std::string& dataFileName,
                     const cs::LidarDataType& xmlStructure)
{
    // todo: check dataFileName.extention() is consistent with xmlStructure.attributes().dataFormat()
    path xmlFilePath(dataFileName);
    xmlFilePath.replace_extension(".xml");
    xml_schema::NamespaceInfomap map;
    map[""].name = "cs";
    //map[""].schema = "/src/LidarFormat/models/xsd/format_me.xsd";
    std::ofstream ofs(xmlFilePath.string().c_str());
    cs::lidarData(ofs, xmlStructure, map);

    // create appropriate writer thanks to the factory
    boost::shared_ptr<LidarFileIO> writer = LidarIOFactory::instance().createObject(xmlStructure.attributes().dataFormat());
    writer->save(lidarContainer, xmlStructure, dataFileName);
}

// BV: create extention based on format
void LidarFile::save(const LidarDataContainer& lidarContainer,
                     const std::string& xmlFileName,
                     const LidarCenteringTransfo& transfo,
                     const cs::DataFormatType format)
{
    std::string ext;
    switch(format)
    {
    case cs::DataFormatType::binary: ext=".bin"; break;
    case cs::DataFormatType::ascii: ext=".txt"; break;
    case cs::DataFormatType::plyarchi: ext=".ply"; break;
    case cs::DataFormatType::las: ext=".las"; break;
    case cs::DataFormatType::terrabin: ext=".terrabin"; break;
    default: ext=".bin";
    }
    path dataFilePath(xmlFileName);
    dataFilePath.replace_extension(ext);

    shared_ptr<cs::LidarDataType> xmlStructure = createXMLStructure(lidarContainer, dataFilePath.string(), transfo, format);

    save(lidarContainer, dataFilePath.string(), *xmlStructure);
}

void LidarFile::save(const LidarDataContainer& lidarContainer,
                     const std::string& xmlFileName,
                     const cs::DataFormatType format)
{
    double x=0., y=0.;
    lidarContainer.getCenteringTransfo(x,y);
    save(lidarContainer, xmlFileName, LidarCenteringTransfo(x,y), format);
}

// BV: guess format from given extention
void LidarFile::save(const LidarDataContainer& lidarContainer,
                     const std::string& dataFileName,
                     const LidarCenteringTransfo& transfo)
{
    // format should be inferred from dataFileName extension
    path dataFilePath(dataFileName);
    std::string ext = dataFilePath.extension().string();
    cs::DataFormatType format = cs::DataFormatType::binary; // default (includes .xml and .bin)
    if(".txt" == ext) format = cs::DataFormatType::ascii;
    else if(".ply" == ext) format = cs::DataFormatType::plyarchi;
    else if(".asc" == ext) format = cs::DataFormatType::ascii;
    else if(".terrabin" == ext) format = cs::DataFormatType::terrabin;
    else if(".las" == ext) format = cs::DataFormatType::las;
    // if user gives .xml filename without specifying format, assume he wants binary
    else if(".xml" == ext) dataFilePath.replace_extension(".bin");

    shared_ptr<cs::LidarDataType> xmlStructure = createXMLStructure(lidarContainer, dataFilePath.string(), transfo, format);

    save(lidarContainer, dataFilePath.string(), *xmlStructure);
}

void LidarFile::save(const LidarDataContainer& lidarContainer,
                     const std::string& dataFileName)
{
    double x=0., y=0.;
    lidarContainer.getCenteringTransfo(x,y);
    save(lidarContainer, dataFileName, LidarCenteringTransfo(x,y));
}


void LidarFile::saveInPlace(const LidarDataContainer& lidarContainer,
                            const std::string& xmlFileName)
{
    std::auto_ptr<cs::LidarDataType> xmlData(cs::lidarData(xmlFileName, xml_schema::Flags::dont_validate));

    const cs::DataFormatType format = xmlData->attributes().dataFormat();
    const std::string dataFileName = (path(xmlFileName).branch_path() / (basename(xmlFileName) + ".bin")).string();

    // create writer appropriate to format using the factory
    boost::shared_ptr<LidarFileIO> writer = LidarIOFactory::instance().createObject(format);
    writer->save(lidarContainer, *xmlData, dataFileName);
}



void LidarFile::loadMetaDataFromXML()
{
    // BV: pas besoin de dupliquer une information qu'on a déj� , en plus on fige la structure donc on perd tout l'interet de xsd
    //parcours du fichier xml et récupération des métadonnées sur les attributs
    //	m_lidarMetaData.ptSize_ = 0;
    /*cs::LidarDataType::AttributesType attributes = m_xmlData->attributes();
    for (cs::LidarDataType::AttributesType::AttributeIterator itAttribute = attributes.attribute().begin(); itAttribute != attributes.attribute().end(); ++itAttribute)
    {
        //TODO adapter si on ne load pas tout
        m_attributeMetaData.push_back( XMLAttributeMetaData(itAttribute->name(), itAttribute->dataType(), true,
                                                            !(itAttribute->min().present() && itAttribute->max().present()),
                                                            itAttribute->min().get(), itAttribute->max().get()));

    }*/

    //meta données générales
    m_lidarMetaData.binaryDataFileName_ = getBinaryDataFileName();
    m_lidarMetaData.nbPoints_ = (size_t)m_xmlData->attributes().dataSize(); //tailleFicOctets/m_lidarMetaData.ptSize_;

    //	std::cout << "taille d'un enregistrement : " << m_lidarMetaData.ptSize_ << std::endl;
    //	double reste = double(tailleFicOctets)/double(m_lidarMetaData.ptSize_) - m_lidarMetaData.nbPoints_;
    //	std::cout << "reste : " << reste << std::endl;
    //	if(reste > 0)
    //		throw std::logic_error("Erreur dans LidarDataContainer::loadMetaDataFromXML : la structure d'attributs du fichier xml ne correspond pas au contenu du fichier binaire ! \n");

    //	//TODO passer la taille en attribut optionnel du xsd
    //	if(attributes.dataSize()>0)
    //		if(attributes.dataSize() != m_lidarMetaData.nbPoints_)
    //			throw std::logic_error("Erreur dans LidarDataContainer::loadMetaDataFromXML : le nb de points du fichier xml ne correspond pas au contenu du fichier binaire ! \n");
}



void LidarFile::setMapsFromXML(LidarDataContainer& lidarContainer) const
{
    lidarContainer.setMapsFromXML(m_xmlData);
}

LidarFile::~LidarFile()
{

}


} //namespace Lidar

