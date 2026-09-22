#include "editor_storage.hpp"
#include <nlohmann/json.hpp>
#include <QBuffer>
#include <QFile>
#include <QImageReader>
#include <QSaveFile>
namespace trafficsim {
ProjectDocument readEditorDocument(const QString& file) {
    QFile input(file);
    if(!input.open(QIODevice::ReadOnly) || input.size()>48*1024*1024)throw std::runtime_error("EDIT_FILE_READ");
    const auto bytes=input.readAll();
    ProjectDocument document;
    try {document=parseDocument(Json::parse(bytes.constData(),bytes.constData()+bytes.size()));}
    catch(const Json::exception&){throw std::runtime_error("EDIT_FILE_FORMAT");}
    if(!document.background.pngBase64->empty()) {
        auto encoded=QByteArray::fromBase64Encoding(QByteArray::fromStdString(*document.background.pngBase64),QByteArray::AbortOnBase64DecodingErrors);
        if(!encoded)throw std::runtime_error("EDIT_BACKGROUND_INVALID");
        QBuffer imageBytes(&encoded.decoded);imageBytes.open(QIODevice::ReadOnly);QImageReader reader(&imageBytes,"PNG");
        const auto size=reader.size();
        if(!size.isValid() || static_cast<qint64>(size.width())*size.height()>32000000 || reader.read().isNull())
            throw std::runtime_error("EDIT_BACKGROUND_INVALID");
    }
    return document;
}
void writeEditorDocument(const QString& file,const Json& value) {
    const auto bytes=value.dump(2)+"\n";
    if(bytes.size()>48*1024*1024)throw std::runtime_error("EDIT_FILE_TOO_LARGE");
    QSaveFile output(file);output.setDirectWriteFallback(false);
    if(!output.open(QIODevice::WriteOnly) || output.write(bytes.data(),static_cast<qint64>(bytes.size()))!=static_cast<qint64>(bytes.size()) || !output.commit())
        throw std::runtime_error("EDIT_FILE_WRITE");
}
}
