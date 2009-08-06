
#include "metaiooggvorbis.h"
#include "metadata.h"

#include <mythtv/mythcontext.h>

MetaIOOggVorbis::MetaIOOggVorbis(void)
    : MetaIOTagLib(".ogg")
{
}

MetaIOOggVorbis::~MetaIOOggVorbis(void)
{
}

TagLib::Ogg::Vorbis::File *MetaIOOggVorbis::OpenFile(const QString &filename)
{
    QByteArray fname = filename.toLocal8Bit();
    TagLib::Ogg::Vorbis::File *oggfile =
                            new TagLib::Ogg::Vorbis::File(fname.constData());
    
    if (!oggfile->isOpen())
    {
        delete oggfile;
        oggfile = NULL;
    }
    
    return oggfile;
}

/*!
 * \brief Writes metadata back to a file
 *
 * \param mdata A pointer to a Metadata object
 * \param exclusive Flag to indicate if only the data in mdata should be
 *                  in the file. If false, any unrecognised tags already
 *                  in the file will be maintained.
 * \returns Boolean to indicate success/failure.
 */
bool MetaIOOggVorbis::write(Metadata* mdata, bool exclusive)
{
    (void) exclusive;

    if (!mdata)
        return false;

    TagLib::Ogg::Vorbis::File *oggfile = OpenFile(mdata->Filename());
    
    if (!oggfile)
        return false;
    
    TagLib::Ogg::XiphComment *tag = oggfile->tag();
    
    if (!tag)
    {
        delete oggfile;
        return false;
    }
    
    WriteGenericMetadata(tag, mdata);

    // Compilation
    if (mdata->Compilation())
    {
        tag->addField("MUSICBRAINZ_ALBUMARTISTID",
                          MYTH_MUSICBRAINZ_ALBUMARTIST_UUID, true);
        tag->addField("COMPILATION_ARTIST",
                        QStringToTString(mdata->CompilationArtist()), true);
    }
    else
    {
        // Don't remove the musicbrainz field unless it indicated a compilation
        if (tag->contains("MUSICBRAINZ_ALBUMARTISTID") &&
            (tag->fieldListMap()["MUSICBRAINZ_ALBUMARTISTID"].toString() ==
                MYTH_MUSICBRAINZ_ALBUMARTIST_UUID))
        {
            tag->removeField("MUSICBRAINZ_ALBUMARTISTID");
        }
        tag->removeField("COMPILATION_ARTIST");
    }

    bool result = oggfile->save();

    if (oggfile)
        delete oggfile;

    return (result);
}

/*!
 * \brief Reads Metadata from a file.
 *
 * \param filename The filename to read metadata from.
 * \returns Metadata pointer or NULL on error
 */
Metadata* MetaIOOggVorbis::read(QString filename)
{
    TagLib::Ogg::Vorbis::File *oggfile = OpenFile(filename);
    
    if (!oggfile)
        return NULL;
    
    TagLib::Ogg::XiphComment *tag = oggfile->tag();
    
    if (!tag)
    {
        delete oggfile;
        return NULL;
    }
    
    Metadata *metadata = new Metadata(filename);
    
    ReadGenericMetadata(tag, metadata);
    
    bool compilation = false;

    if (tag->contains("COMPILATION_ARTIST"))
    {
        QString compilation_artist = TStringToQString(
            tag->fieldListMap()["COMPILATION_ARTIST"].toString()).trimmed();
        if (compilation_artist != metadata->Artist())
        {
            metadata->setCompilationArtist(compilation_artist);
            compilation = true;
        }
    }
    
    if (!compilation && tag->contains("MUSICBRAINZ_ALBUMARTISTID"))
    {
        QString musicbrainzcode = TStringToQString(
        tag->fieldListMap()["MUSICBRAINZ_ALBUMARTISTID"].toString()).trimmed();
        if (musicbrainzcode == MYTH_MUSICBRAINZ_ALBUMARTIST_UUID)
            compilation = true;
    }

    metadata->setCompilation(compilation);

    if (metadata->Length() <= 0)
    {
        TagLib::FileRef *fileref = new TagLib::FileRef(oggfile);
        metadata->setLength(getTrackLength(fileref));
        // FileRef takes ownership of oggfile, and is responsible for it's
        // deletion. Messy.
        delete fileref;
    }
    else
        delete oggfile;
    
    return metadata;
}
