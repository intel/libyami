#!/bin/sh

MV="mv -f"

replace()
{
    if [ ! -f "$1" ]; then
    	echo "can't read $1: No such file or directory"
        exit 1
    fi
    
    sed -i '/[<]gst\/gstconfig.h[>]/d' "$1"
	sed -i 's/[<]gst\/codecparsers\/gstmpegvideoparser.h[>]/"mpegvideoparser.h"/g' "$1"
    sed -i 's/[<]gst\/codecparsers\/gstvp8parser.h[>]/"vp8parser.h"/g' "$1"
    sed -i 's/[<]glib.h[>]/"commondef.h"/g' "$1"
    sed -i 's/guint##bits/uint##bits##_t/g' "$1"
    
    #rename temporarily, will be recovery at the end of this script
    sed -i 's/GST_READ_##upper/YAMI_READ_##upper/g' "$1"
    sed -i 's/GST_READ_UINT##bits/YAMI_READ_UINT##bits/g' "$1"
    sed -i 's/GST_PUT/YAMI_PUT/g' "$1"
    sed -i 's/GST_WRITE/YAMI_WRITE/g' "$1"
    sed -i 's/GST_READ/YAMI_READ/g' "$1"
    sed -i 's/_gst_debug_category_new/_yami_debug_category_new/g' "$1"
    
	sed -i 's/[<]gst\/gst.h[>]/"gst\/gst.h"/g' "$1"
    sed -i 's/\<GST_LOG\>/INFO/g' "$1"
    #remove gst and gst_, ignore case 
    sed -i "s/[gG][sS][Tt]_\?//g" "$1"
    
    #replace data type integer
    sed -i 's/gboolean/bool/g' "$1"  
    sed -i 's/gchar/char/g' "$1"  
    sed -i 's/guchar/unsigned char/g' "$1"  
    sed -i 's/\<guint8\>/uint8_t/g' "$1" 
    sed -i 's/\<gint8\>/int8_t/g' "$1" 
    sed -i 's/\<gint16\>/int16_t/g' "$1" 
    sed -i 's/\<guint16\>/uint16_t/g' "$1" 
    sed -i 's/\<gint\>/int32_t/g' "$1"   
    sed -i 's/\<guint\>/uint32_t/g' "$1" 
    sed -i 's/\<gint32\>/int32_t/g' "$1" 
    sed -i 's/\<guint32\>/uint32_t/g' "$1" 
    sed -i 's/\<gsize\>/size_t/g' "$1" 
    sed -i 's/\<guint64\>/uint64_t/g' "$1" 
    sed -i 's/\<gint64\>/int64_t/g' "$1" 
    #float
    sed -i 's/gfloat/float/g' "$1"  
    sed -i 's/\<gdouble\>/double/g' "$1" 
    #pointer
    sed -i 's/\<gpointer\>/void*/g' "$1" 
    sed -i 's/\<gconstpointer\>/const void*/g' "$1" 
    #include header file 
    sed -i 's/[<]\/base\/bitreader.h[>]/"bitreader.h"/g' "$1"
    sed -i 's/[<]\/base\/bytereader.h[>]/"bytereader.h"/g' "$1"
    sed -i 's/[<]\/base\/bitwriter.h[>]/"bitwriter.h"/g' "$1"
    sed -i 's/[<]\/base\/bytewriter.h[>]/"bytewriter.h"/g' "$1"
    
    #recovery change at the begining
    sed -i 's/YAMI_READ_##upper/GST_READ_##upper/g' "$1"
    sed -i 's/YAMI_READ_UINT##bits/GST_READ_UINT##bits/g' "$1"
    sed -i 's/YAMI_PUT/GST_PUT/g' "$1"
    sed -i 's/YAMI_WRITE/GST_WRITE/g' "$1"
    sed -i 's/YAMI_READ/GST_READ/g' "$1"
    sed -i 's/_yami_debug_category_new/_gst_debug_category_new/g' "$1"
    
    #change file name
    RENAME=`echo "$1" | sed "s/[g,G][s,S][T,t]_\?//g"`
    if [ "$1" != "$RENAME" ]; then
        echo "Rename file $1 to $RENAME"
        $MV "$1" "$RENAME"
    fi
}

for file in `ls`; do
	if [ "$file" != "replace.sh" ]; then
        echo "Begin -------------- $file ----------"
		replace $file
		echo "End ---\n"
	fi
done
