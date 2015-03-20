/*
 * Copyright 2005-2014 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */

#include "grib_api_internal.h"

/*
   This is used by make_class.pl

   START_CLASS_DEF
   CLASS      = accessor
   SUPER      = grib_accessor_class_values
   IMPLEMENTS = init
   IMPLEMENTS = unpack_double
   IMPLEMENTS = pack_double
   IMPLEMENTS = value_count
   MEMBERS=const char*   number_of_values
   MEMBERS=const char*   reference_value
   MEMBERS=const char*   binary_scale_factor
   MEMBERS=const char*   decimal_scale_factor
   MEMBERS=const char*   bits_per_value

   MEMBERS=const char*   number_of_data_points

   MEMBERS=const char*   szip_options_mask
   MEMBERS=const char*   szip_bits_per_pixel
   MEMBERS=const char*   szip_pixels_per_block
   MEMBERS=const char*   szip_pixels_per_scanline

   END_CLASS_DEF

 */

/* START_CLASS_IMP */

/*

Don't edit anything between START_CLASS_IMP and END_CLASS_IMP
Instead edit values between START_CLASS_DEF and END_CLASS_DEF
or edit "accessor.class" and rerun ./make_class.pl

*/

static int pack_double(grib_accessor*, const double* val,size_t *len);
static int unpack_double(grib_accessor*, double* val,size_t *len);
static long value_count(grib_accessor*);
static void init(grib_accessor*,const long, grib_arguments* );
static void init_class(grib_accessor_class*);

typedef struct grib_accessor_data_szip_packing {
    grib_accessor          att;
/* Members defined in gen */
/* Members defined in values */
	int  carg;
	const char* seclen;
	const char* offsetdata;
	const char* offsetsection;
	int dirty;
/* Members defined in data_szip_packing */
	const char*   number_of_values;
	const char*   reference_value;
	const char*   binary_scale_factor;
	const char*   decimal_scale_factor;
	const char*   bits_per_value;
	const char*   number_of_data_points;
	const char*   szip_options_mask;
	const char*   szip_bits_per_pixel;
	const char*   szip_pixels_per_block;
	const char*   szip_pixels_per_scanline;
} grib_accessor_data_szip_packing;

extern grib_accessor_class* grib_accessor_class_values;

static grib_accessor_class _grib_accessor_class_data_szip_packing = {
    &grib_accessor_class_values,                      /* super                     */
    "data_szip_packing",                      /* name                      */
    sizeof(grib_accessor_data_szip_packing),  /* size                      */
    0,                           /* inited */
    &init_class,                 /* init_class */
    &init,                       /* init                      */
    0,                  /* post_init                      */
    0,                    /* free mem                       */
    0,                       /* describes himself         */
    0,                /* get length of section     */
    0,              /* get length of string      */
    &value_count,                /* get number of values      */
    0,                 /* get number of bytes      */
    0,                /* get offset to bytes           */
    0,            /* get native type               */
    0,                /* get sub_section                */
    0,               /* grib_pack procedures long      */
    0,               /* grib_pack procedures long      */
    0,                  /* grib_pack procedures long      */
    0,                /* grib_unpack procedures long    */
    &pack_double,                /* grib_pack procedures double    */
    &unpack_double,              /* grib_unpack procedures double  */
    0,                /* grib_pack procedures string    */
    0,              /* grib_unpack procedures string  */
    0,                 /* grib_pack procedures bytes     */
    0,               /* grib_unpack procedures bytes   */
    0,            /* pack_expression */
    0,              /* notify_change   */
    0,                /* update_size   */
    0,            /* preferred_size   */
    0,                    /* resize   */
    0,      /* nearest_smaller_value */
    0,                       /* next accessor    */
    0,                    /* compare vs. another accessor   */
    0,     /* unpack only ith value          */
    0,     /* unpack a subarray         */
    0,             		/* clear          */
};


grib_accessor_class* grib_accessor_class_data_szip_packing = &_grib_accessor_class_data_szip_packing;


static void init_class(grib_accessor_class* c)
{
	c->dump	=	(*(c->super))->dump;
	c->next_offset	=	(*(c->super))->next_offset;
	c->string_length	=	(*(c->super))->string_length;
	c->byte_count	=	(*(c->super))->byte_count;
	c->byte_offset	=	(*(c->super))->byte_offset;
	c->get_native_type	=	(*(c->super))->get_native_type;
	c->sub_section	=	(*(c->super))->sub_section;
	c->pack_missing	=	(*(c->super))->pack_missing;
	c->is_missing	=	(*(c->super))->is_missing;
	c->pack_long	=	(*(c->super))->pack_long;
	c->unpack_long	=	(*(c->super))->unpack_long;
	c->pack_string	=	(*(c->super))->pack_string;
	c->unpack_string	=	(*(c->super))->unpack_string;
	c->pack_bytes	=	(*(c->super))->pack_bytes;
	c->unpack_bytes	=	(*(c->super))->unpack_bytes;
	c->pack_expression	=	(*(c->super))->pack_expression;
	c->notify_change	=	(*(c->super))->notify_change;
	c->update_size	=	(*(c->super))->update_size;
	c->preferred_size	=	(*(c->super))->preferred_size;
	c->resize	=	(*(c->super))->resize;
	c->nearest_smaller_value	=	(*(c->super))->nearest_smaller_value;
	c->next	=	(*(c->super))->next;
	c->compare	=	(*(c->super))->compare;
	c->unpack_double_element	=	(*(c->super))->unpack_double_element;
	c->unpack_double_subarray	=	(*(c->super))->unpack_double_subarray;
	c->clear	=	(*(c->super))->clear;
}

/* END_CLASS_IMP */

static void init(grib_accessor* a,const long v, grib_arguments* args)
{
    grib_accessor_data_szip_packing *self =(grib_accessor_data_szip_packing*)a;

    self->number_of_values      = grib_arguments_get_name(a->parent->h,args,self->carg++);
    self->reference_value       = grib_arguments_get_name(a->parent->h,args,self->carg++);
    self->binary_scale_factor   = grib_arguments_get_name(a->parent->h,args,self->carg++);
    self->decimal_scale_factor  = grib_arguments_get_name(a->parent->h,args,self->carg++);
    self->bits_per_value           = grib_arguments_get_name(a->parent->h,args,self->carg++);

    self->number_of_data_points  = grib_arguments_get_name(a->parent->h,args,self->carg++);


    self->szip_options_mask  = grib_arguments_get_name(a->parent->h,args,self->carg++);
    self->szip_bits_per_pixel  = grib_arguments_get_name(a->parent->h,args,self->carg++);
    self->szip_pixels_per_block  = grib_arguments_get_name(a->parent->h,args,self->carg++);
    self->szip_pixels_per_scanline  = grib_arguments_get_name(a->parent->h,args,self->carg++);

    a->flags |= GRIB_ACCESSOR_FLAG_DATA;

}

static long value_count(grib_accessor* a)
{
    grib_accessor_data_szip_packing *self =(grib_accessor_data_szip_packing*)a;
    long n_vals= 0;
    if(grib_get_long_internal(a->parent->h,self->number_of_values,&n_vals) != GRIB_SUCCESS)
        return 0;

    return n_vals;
}

#ifdef HAVE_LIBSZIP

#include "szlib.h"


static int  unpack_double(grib_accessor* a, double* val, size_t *len)
{

    grib_accessor_data_szip_packing *self =(grib_accessor_data_szip_packing*)a;

    int err = GRIB_SUCCESS;
    int i,j;
    size_t buflen = grib_byte_count(a);
    SZ_com_t sz_options;
    double bscale = 0;
    double dscale = 0;
    unsigned char* buf = NULL;
    size_t n_vals = 0;
    size_t size;
    unsigned char* decoded = NULL;
    unsigned char *p = NULL;
    long pos = 0;

    long binary_scale_factor = 0;
    long decimal_scale_factor = 0;
    double reference_value = 0;
    long bits_per_value =0;
    long bits8;


    long szip_options_mask ;
    long szip_bits_per_pixel ;
    long szip_pixels_per_block  ;
    long szip_pixels_per_scanline  ;

    self->dirty=0;

    n_vals = grib_value_count(a);

    if((err = grib_get_long_internal(a->parent->h,self->bits_per_value,&bits_per_value)) != GRIB_SUCCESS)
        return err;
    if((err = grib_get_double_internal(a->parent->h,self->reference_value, &reference_value)) != GRIB_SUCCESS)
        return err;
    if((err = grib_get_long_internal(a->parent->h,self->binary_scale_factor, &binary_scale_factor)) != GRIB_SUCCESS)
        return err;
    if((err = grib_get_long_internal(a->parent->h,self->decimal_scale_factor, &decimal_scale_factor)) != GRIB_SUCCESS)
        return err;


    if((err = grib_get_long_internal(a->parent->h,self->szip_options_mask,&szip_options_mask)) != GRIB_SUCCESS)
        return err;
    if((err = grib_get_long_internal(a->parent->h,self->szip_bits_per_pixel, &szip_bits_per_pixel)) != GRIB_SUCCESS)
        return err;
    if((err = grib_get_long_internal(a->parent->h,self->szip_pixels_per_block, &szip_pixels_per_block)) != GRIB_SUCCESS)
        return err;
    if((err = grib_get_long_internal(a->parent->h,self->szip_pixels_per_scanline, &szip_pixels_per_scanline)) != GRIB_SUCCESS)
        return err;


    bscale = grib_power(binary_scale_factor,2);
    dscale = grib_power(-decimal_scale_factor,10);

    /* TODO: This should be called upstream */
    if(*len < n_vals)
        return GRIB_ARRAY_TOO_SMALL;

    /* Special case */

    if(bits_per_value == 0)
    {
        for(i = 0; i < n_vals; i++)
            val[i] = reference_value;
        *len = n_vals;
        return GRIB_SUCCESS;
    }

    buf = (unsigned char*)a->parent->h->buffer->data;
    buf += grib_byte_offset(a);

    memset(&sz_options, 0, sizeof(sz_options));
    sz_options.options_mask         = szip_options_mask;
    sz_options.bits_per_pixel       = szip_bits_per_pixel;
    sz_options.pixels_per_block     = szip_pixels_per_block;
    sz_options.pixels_per_scanline  = szip_pixels_per_scanline;

/*
    printf("sz_options.options_mask %d\n", sz_options.options_mask);
    printf("sz_options.bits_per_pixel %d\n", sz_options.bits_per_pixel);
    printf("sz_options.pixels_per_block %d\n", sz_options.pixels_per_block);
    printf("sz_options.pixels_per_scanline %d\n", sz_options.pixels_per_scanline);
    */

    bits8 = ((szip_bits_per_pixel + 7)/8)*8;
    size = n_vals * (szip_bits_per_pixel + 7)/8;
    decoded = grib_context_buffer_malloc_clear(a->parent->h->context,size);
    if(!decoded) {
        err = GRIB_OUT_OF_MEMORY;
        goto cleanup;
    }


    if((err = SZ_BufftoBuffDecompress( decoded,&size,buf, buflen , &sz_options)) != SZ_OK)
    {
        printf("SZ_BufftoBuffDecompress Error %d\n", err);
        err = GRIB_ENCODING_ERROR;
        goto cleanup;
    }

    /* printf("bscale=%g dscale=%g reference_value=%g\n",bscale,dscale,reference_value); */


    pos = 0;
    p = decoded;
    for(i = 0; i < n_vals; i++)
    {
        val[i] = (double) (((grib_decode_unsigned_long(p, &pos, bits8)*bscale)+reference_value)*dscale);
    }
    /*-------------------------------------------*/
    *len = n_vals;


cleanup:
    grib_context_buffer_free(a->parent->h->context,decoded);
    return err;

}

static int pack_double(grib_accessor* a, const double* val, size_t *len)
{
    grib_accessor_data_szip_packing *self =(grib_accessor_data_szip_packing*)a;

    int err = GRIB_SUCCESS;
    int i;
    size_t buflen = grib_byte_count(a);

    unsigned char* buf = NULL;
    unsigned char* encoded = NULL;
    size_t n_vals = 0;

    long binary_scale_factor = 0;
    long decimal_scale_factor = 0;
    double reference_value = 0;
    long bits8;
    long bits_per_value =0;
    double max,min;

    double d;


    unsigned char *p;
    double divisor;

    long ni,nj;
    long scanning_mode;
    long list_defining_points;
    long number_of_data_points;

    long szip_options_mask ;
    long szip_bits_per_pixel ;
    long szip_pixels_per_block  ;
    long szip_pixels_per_scanline  ;

    size_t width, height;

    SZ_com_t sz_options;

    self->dirty=1;

    n_vals = grib_value_count(a);

    if((err = grib_get_long_internal(a->parent->h,self->bits_per_value,&bits_per_value)) != GRIB_SUCCESS)
        return err;
    if((err = grib_get_double_internal(a->parent->h,self->reference_value, &reference_value)) != GRIB_SUCCESS)
        return err;
    if((err = grib_get_long_internal(a->parent->h,self->binary_scale_factor, &binary_scale_factor)) != GRIB_SUCCESS)
        return err;
    if((err = grib_get_long_internal(a->parent->h,self->decimal_scale_factor, &decimal_scale_factor)) != GRIB_SUCCESS)
        return err;


    if((err = grib_get_long_internal(a->parent->h,self->szip_options_mask,&szip_options_mask)) != GRIB_SUCCESS)
        return err;
    if((err = grib_get_long_internal(a->parent->h,self->szip_bits_per_pixel, &szip_bits_per_pixel)) != GRIB_SUCCESS)
        return err;
    if((err = grib_get_long_internal(a->parent->h,self->szip_pixels_per_block, &szip_pixels_per_block)) != GRIB_SUCCESS)
        return err;
    if((err = grib_get_long_internal(a->parent->h,self->szip_pixels_per_scanline, &szip_pixels_per_scanline)) != GRIB_SUCCESS)
        return err;


    /* Special case */

    if(*len == 0) {
        grib_buffer_replace(a, NULL, 0,1,1);
        return GRIB_SUCCESS;
    }

    if(bits_per_value == 0)
    {
        int i;
        /* constant field */
        for(i = 1 ; i < n_vals; i++)
            Assert(val[i] == val[0]);

        if((err = grib_set_double_internal(a->parent->h,self->reference_value, val[0])) != GRIB_SUCCESS)
            return err;
        {
            /* Make sure we can decode it again */
            double ref = 1e-100;
            grib_get_double_internal(a->parent->h,self->reference_value,&ref);
            printf("%g %g %g\n",reference_value,ref,reference_value-ref);
            Assert(ref == reference_value);
        }

        if((err = grib_set_long_internal(a->parent->h,self->number_of_values, n_vals)) !=  GRIB_SUCCESS)
            return err;

        grib_buffer_replace(a, NULL, 0,1,1);

        return GRIB_SUCCESS;
    }

    if((err = grib_get_long_internal(a->parent->h,self->number_of_data_points,&number_of_data_points)) != GRIB_SUCCESS)
        return err;




    d = grib_power(decimal_scale_factor,10) ;


    max = val[0];
    min = max;
    for(i=1;i< n_vals;i++)
    {
        if (val[i] > max )
            max = val[i];
        if (val[i] < min )
            min = val[i];
    }
    min *= d;
    max *= d;

    if (grib_get_nearest_smaller_value(a->parent->h,self->reference_value,min,&reference_value)
            !=GRIB_SUCCESS) {
        grib_context_log(a->parent->h->context,GRIB_LOG_ERROR,
                         "unable to find nearest_smaller_value of %g for %s",min,self->reference_value);
        exit(GRIB_INTERNAL_ERROR);
    }

    if(reference_value > min)
    {
        fprintf(stderr,"reference_value=%g min_value=%g diff=%g\n",reference_value,min,reference_value-min);
        Assert(reference_value <= min);
    }

    binary_scale_factor = grib_get_binary_scale_fact(max,reference_value,bits_per_value,&err);
    divisor = grib_power(-binary_scale_factor,2);

    bits8 = (bits_per_value+7)/8*8;
    encoded = grib_context_buffer_malloc_clear(a->parent->h->context,bits8/8*n_vals);

    if(!encoded) {
        err = GRIB_OUT_OF_MEMORY;
        goto cleanup;
    }

    buflen = 0;
    p = encoded;
    for(i=0;i< n_vals;i++){
        long blen = bits8;
        unsigned long unsigned_val = (unsigned long)((((val[i]*d)-(reference_value))*divisor)+0.5);
        while(blen >= 8)
        {
            blen   -= 8;
            *p = (unsigned_val >> blen);
            p++;
            buflen++;
        }
    }
    /*       buflen = n_vals*(bits_per_value/8);*/

    grib_context_log(a->parent->h->context, GRIB_LOG_DEBUG,
                     "grib_accessor_data_szip_packing : pack_double : packing %s, %d values", a->name, n_vals);

    buflen += 10240;
    buf = grib_context_buffer_malloc_clear(a->parent->h->context,buflen);

    if(!buf) {
        err = GRIB_OUT_OF_MEMORY;
        goto cleanup;
    }


    if((err = grib_set_double_internal(a->parent->h,self->reference_value, reference_value)) != GRIB_SUCCESS)
        return err;
    {
        /* Make sure we can decode it again */
        double ref = 1e-100;
        grib_get_double_internal(a->parent->h,self->reference_value,&ref);
        Assert(ref == reference_value);
    }

    if((err = grib_set_long_internal(a->parent->h,self->binary_scale_factor, binary_scale_factor)) != GRIB_SUCCESS)
        return err;

    if((err = grib_set_long_internal(a->parent->h,self->decimal_scale_factor, decimal_scale_factor)) != GRIB_SUCCESS)
        return err;


    memset(&sz_options, 0, sizeof(sz_options));
    sz_options.options_mask         = szip_options_mask;
    sz_options.bits_per_pixel       = bits_per_value;
    sz_options.pixels_per_block     = szip_pixels_per_block;
    sz_options.pixels_per_scanline  = szip_pixels_per_scanline;

    /*

        This does not support spherical harmonics, and treats 24 differently than:
        see http://cdo.sourcearchive.com/documentation/1.5.1.dfsg.1-1/cgribexlib_8c_source.html
    */

    /*
    printf("sz_options.options_mask %d\n", sz_options.options_mask);
    printf("sz_options.bits_per_pixel %d\n", sz_options.bits_per_pixel);
    printf("sz_options.pixels_per_block %d\n", sz_options.pixels_per_block);
    printf("sz_options.pixels_per_scanline %d\n", sz_options.pixels_per_scanline);
    */

    if((err = SZ_BufftoBuffCompress(buf,&buflen,encoded, bits8/8*n_vals, &sz_options)) != SZ_OK)
    {
        printf("SZ_BufftoBuffCompress Error %d\n", err);
        err = GRIB_ENCODING_ERROR;
        goto cleanup;
    }

    /*

    printf("n_vals = %ld, bits8 = %ld\n", (long)n_vals, (long)bits8);
    printf("in %ld out => %ld\n", (long)bits8/8*n_vals,(long) buflen);
    */

    grib_buffer_replace(a, buf, buflen ,1,1);

cleanup:
    grib_context_buffer_free(a->parent->h->context,buf);
    grib_context_buffer_free(a->parent->h->context,encoded);

    if(err == GRIB_SUCCESS)
        err = grib_set_long_internal(a->parent->h,self->number_of_values, *len);

    if(err == GRIB_SUCCESS)
        err = grib_set_long_internal(a->parent->h,self->szip_bits_per_pixel, sz_options.bits_per_pixel);

    return err;
}
#else

static int  unpack_double(grib_accessor* a, double* val, size_t *len)
{
    grib_context_log(a->parent->h->context, GRIB_LOG_ERROR,
                     "grib_accessor_data_szip_packing : szip support no enabled. Please rerun configure with --with-szip-support");
    return GRIB_NOT_IMPLEMENTED;
}

static int pack_double(grib_accessor* a, const double* val, size_t *len)
{
    grib_context_log(a->parent->h->context, GRIB_LOG_ERROR,
                     "grib_accessor_data_szip_packing : szip support no enabled. Please rerun configure with --with-szip-support");
    return GRIB_NOT_IMPLEMENTED;
}

#endif
