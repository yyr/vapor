//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		DVRVolumizer.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Implementation of the DVRVolumizer class
//		Modified from code by John Clyne
//		Modified to support a volumizer renderer inside a QT window
//
#ifdef VOLUMIZER
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include "glutil.h"
#include "qgl.h"
#include <Volumizer2/Error.h>
#include <Volumizer2/TMFragmentProgram.h>
#include "vapor/MyBase.h"

#include "DVRVolumizer.h"
#include "renderer.h"


using namespace VAPoR;
static void	deletion_cb(
	vzObject	*,   //object
	void*      //userdata
) {
//	char	*s = (char *) userdata;

//	fprintf(stderr, "Deletion callback : %s\n", s);
}



DVRVolumizer::DVRVolumizer(
	int * ,//argc
	char **, //argv
	int	nthreads,
	Renderer* ren
) {
	int	i;
	myRenderer = ren;
	volume_c = NULL;
	shader_c = NULL;
	appearance_c = NULL;
	shape_c = NULL;
	geometry_c = NULL;
	lightdir_parm_c = NULL;
	ambient_parm_c = NULL;
	diffuse_parm_c = NULL;
	table_c = NULL;
	render_action_c = NULL;
	grad_data_size_c = 0;
	grad_data_c = NULL;
	data_c = NULL;
	do_lighting_c = 0;
	nthreads_c = nthreads;
	init_c = 0;
	is_rendering_c = 0;
	do_frag_prog_c = 0;

	data_dirty_c = 0;
	region_dirty_c = 0;
	clut_dirty_c = 0;
	olut_dirty_c = 0;
	light_dirty_c = 0;

	lightdir_c[0] = -1.0; lightdir_c[1] = 0.0; 
	lightdir_c[2] = 0.0; lightdir_c[3] = 1.0;
	ambient_c[0] = ambient_c[1] = ambient_c[2] = 0.18f;
	diffuse_c[0] = diffuse_c[1] = diffuse_c[2] = 0.5f;

	if (type != UINT8) {
		QString strng("Volumizer error; Unsupported voxel type: ");
		strng += QString::number(type);
		Params::BailOut(strng.ascii(),__FILE__,__LINE__);
	}

	for(i=0; i<256; i++) {
		lut_c[i][0] = (float) i / 255.0;
		lut_c[i][1] = (float) i / 255.0;
		lut_c[i][2] = (float) i / 255.0;
		lut_c[i][3] = (float) i / 255.0;
	}
	for(i=0;i<3;i++) {
		data_dim_c[i] = 0;
	}
	for(i=0;i<6;i++) {
		data_roi_c[i] = 0;
	}
	
	appearance_c = create_appearance();

	// Initialize geometry
	geometry_c = new vzBlock();
	geometry_c->addDeletionCallback(deletion_cb, (void *) "vzBlock");

	// Initialize shape node
	shape_c = new vzShape(geometry_c, appearance_c);
	shape_c->addDeletionCallback(deletion_cb, (void *) "vzShape");
	geometry_c->unref();
	appearance_c->unref();

	init_c = 1;
}

DVRVolumizer::~DVRVolumizer() {

	if (render_action_c && shape_c) {
		render_action_c->unmanage(shape_c);
		//Following crashes on Windows!
#ifndef WIN32
		delete render_action_c;
#endif
		shape_c->unref();
		
	}
	
}

int	DVRVolumizer::GraphicsInit(
) {
	const char	*extensions;

	// see if hardware supports fragment programs. If so, use 'em.
	//
	extensions = (const char *) glGetString( GL_EXTENSIONS );
	if (strstr(extensions, "ARB_fragment_program")) {
		do_frag_prog_c = 1;
		MessageReporter::infoMsg("Using ARB_fragment_program");
	} else {
		MessageReporter::infoMsg("Not using ARB_fragment_program");
	}
	MessageReporter::infoMsg("OpenGL extensions are %s", extensions);
	
	//fflush(stderr);


	// Enable standard back-to-front alpha blending:
	//
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glDisable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);

	// Make sure slices are rendered as solid polygons:
	//
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Initialize the render action
	render_action_c = new vzTMRenderAction(1);
	render_action_c->manage(shape_c);

	return(0);
}


int DVRVolumizer::SetRegion(
	void *data,
	int nx, int ny, int nz,
	const int data_roi[6],
	const float extents[6],
	const int*, int
    
) {
	int	i;

	data_c = (unsigned char *) data;
	if (!data_c) fprintf(stderr, "Invalid data in DVRVolumizer::SetRegion");
	data_dirty_c = 1;

	if (! volume_c) region_dirty_c = 1;

	if (data_dim_c[0] != nx || data_dim_c[1] != ny || data_dim_c[2] != nz ) {

		data_dim_c[0] = nx;
		data_dim_c[1] = ny;
		data_dim_c[2] = nz;
		region_dirty_c = 1;
	}

	for(i=0;i<6;i++) {
		if (data_roi_c[i] != data_roi[i]) {
			data_roi_c[i] = data_roi[i];
			region_dirty_c = 1;
		}
		geom_roi_c[i] = extents[i];
		if (i >2){
			assert(data_roi_c[i] >= data_roi_c[i-3]);
			assert(extents[i] >= extents[i-3]);
		}
	}



	
	if (region_dirty_c) {

		/*
fprintf(
	stderr,"data_dim_c==(%d, %d, %d)\n",
	data_dim_c[0],data_dim_c[1],data_dim_c[2]
);
fprintf(
	stderr,"data_roi_c==(%d, %d, %d) (%d, %d, %d)\n",
	data_roi_c[0],data_roi_c[1],data_roi_c[2],
	data_roi_c[3],data_roi_c[4],data_roi_c[5]
);
fprintf(
	stderr,"geom_roi_c==(%f, %f, %f) (%f, %f, %f)\n",
	geom_roi_c[0],geom_roi_c[1],geom_roi_c[2],
	geom_roi_c[3],geom_roi_c[4],geom_roi_c[5]
);
*/
	}

	return(0);
}




int	DVRVolumizer::Render(
	const float	matrix[16]
) {
	int	i;

	

	if (! data_c) {
		MessageReporter::errorMsg("Volumizer error, no data to render,\npossibly insufficient memory");
		return(-1);
	}

	for(i=0; i<16; i++) {
		matrix_c[i] = matrix[i];
	}

	if (render() < 0) return (-1);
	is_rendering_c = 1;

	return(0);
}

void	DVRVolumizer::PrintOptions(FILE *) {
}

void	DVRVolumizer::SetCLUT(
	const float ctab[256][4]
) {
	int	i;

	for(i=0; i<256; i++) {
		lut_c[i][0] = ctab[i][0];
		lut_c[i][1] = ctab[i][1];
		lut_c[i][2] = ctab[i][2];

		llut_c[i][0] = 4 * lut_c[i][0]; if(llut_c[i][0]>1.0) llut_c[i][0] = 1.0;
		llut_c[i][1] = 4 * lut_c[i][1]; if(llut_c[i][1]>1.0) llut_c[i][1] = 1.0;
		llut_c[i][2] = 4 * lut_c[i][2]; if(llut_c[i][2]>1.0) llut_c[i][2] = 1.0;
	}
	clut_dirty_c = 1;
}


void	DVRVolumizer::SetOLUT(
	const float atab[256][4],
	const int numRefinements
) {
	int	i;
	//Calculate opacity correction
	//Delta is 1 for the fineest refinement, multiplied by 2^n (the sampling distance)
	//If the data is sampled coarser
	double delta = (double)(1<<numRefinements);
	for(i=0; i<256; i++) {
		double opac = atab[i][3];
		opac = 1.0 - pow((1.0 - opac), delta);
		if (opac > 1.0) opac = 1.0;
//Special correction for volumizer/windows (or possibly nvidia.  It seems to 
//truncate to 0 values less than .034
//
#ifdef WIN32
		if (opac > 0.015 && opac < 0.034) opac = 0.034;
#endif
	
		lut_c[i][3] = (float)opac;
		llut_c[i][3] = (float)opac;
	}
	olut_dirty_c = 1;
}

void	DVRVolumizer::SetLightingOnOff(int on) 
{
	if ((do_lighting_c && on) || (!do_lighting_c && !on)) return;

	do_lighting_c = on;
	light_dirty_c = 1;
	olut_dirty_c = 1;
	clut_dirty_c = 1;
	data_dirty_c = 1;

}

void   DVRVolumizer::SetAmbient(float r, float g, float b) 
{
	ambient_c[0] = r; ambient_c[1] = g; ambient_c[2] = b;
	
	float ambient_vec[3] = {ambient_c[0], ambient_c[1], ambient_c[2]};
	ambient_parm_c = new vzParameterVec3f(ambient_vec);
	ambient_parm_c->addDeletionCallback(
		deletion_cb, (void *) "vzParameterVec3f"
	);
	appearance_c->setParameter("ambient", ambient_parm_c);
	ambient_parm_c->unref();
}

void   DVRVolumizer::SetDiffuse(float r, float g, float b) 
{
	diffuse_c[0] = r; diffuse_c[1] = g; diffuse_c[2] = b;

	float diffuse_vec[3] = {diffuse_c[0], diffuse_c[1], diffuse_c[2]};
	diffuse_parm_c = new vzParameterVec3f(diffuse_vec);
	diffuse_parm_c->addDeletionCallback(
		deletion_cb, (void *) "vzParameterVec3f"
	);
	appearance_c->setParameter("diffuse", diffuse_parm_c);
}

void   DVRVolumizer::SetSpecular(float , float, float) 
{}

void	DVRVolumizer::SetPerspectiveOnOff(int ) 
{}

//extern void	OGLCheckState();

int	DVRVolumizer::render()
{
	static	GLfloat	l_inv[4];
	
	// Set up transformations to translate and scale volume to coincide
	// with real world coordinates as specified by geom_roi_c. The 
	// volumizer coordinate system runs from 0.0 to 1.0. Prior to 
	// rendering the coordinates are translated to -0.5 to 0.5. Finally they
	// are translated and scaled as necessary to match the desired world
	// coordinate extents
	//
	
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glLoadIdentity();
	//AN:  Modified this so that we just position the volume at the
	//cube centered at the origin of diameter 1.
	//We could just keep the gl modelview matrix that was present at the calling of 
	//Render() but the lighting in this code seems to need to know the matrix as well
	
	glMultMatrixf(matrix_c);
	//glTranslatef(-0.5, -0.5, -0.5);

	if (clut_dirty_c || olut_dirty_c) {
		table_c->setDataFormat(VZ_RGBA);
		if (do_lighting_c) {
			table_c->setDataPtr(256, llut_c);
		}
		else {
			table_c->setDataPtr(256, lut_c);
		}
		clut_dirty_c = 0;
		olut_dirty_c = 0;
	}

	// update texture if necessary
	//
	if (data_dirty_c) {
		int	do_unref = 0;

		appearance_c = create_appearance();
		shape_c->setAppearance(appearance_c);
		appearance_c->unref();

		render_action_c->unmanage(shape_c);

		if (region_dirty_c) {

			do_unref = 1;

			volume_c = new vzParameterVolumeTexture(
				data_dim_c, data_roi_c, data_c, VZ_UNSIGNED_BYTE,
				VZ_LUMINANCE, VZ_DEFAULT_INTERNAL_FORMAT
			);

			volume_c->addDeletionCallback(deletion_cb,(void *) "vzParameterVolumeTexture");
			volume_c->setGeometryROI(geom_roi_c);

			region_dirty_c = 0;
		}
		else {
			volume_c->setDataPtr(data_c);
		}

		appearance_c->setParameter("volume", volume_c);
		if (do_unref) volume_c->unref();

		if (do_lighting_c) {
			vzParameterVolumeTexture	*gradient_volume;

			gradient_volume = compute_gradient_volume((unsigned char *) data_c);
			if (! gradient_volume) return(-1);

			appearance_c->setParameter("gradient", gradient_volume);
			gradient_volume->unref();

		}
		//qWarning("ready to call manage()");
		render_action_c->manage(shape_c);
		//qWarning("after manage()");
		data_dirty_c = 0;
	}

	// compute direction for lights if enabled -- lighting position is 
	// fixed relative to viewpoint position
	//
	if (do_lighting_c) {
		GLfloat	m_inv[16];
		//int	matrix4x4_inverse(const GLfloat	*in, GLfloat *out);
		//void matrix4x4_vec3_mult(
		//	const GLfloat	m[16], const GLfloat a[4],
		//	GLfloat b[4]
		//);

		if (matrix4x4_inverse(matrix_c, m_inv) < 0) {
			MessageReporter::errorMsg(
			"Rendering Error, singular Transformation Matrix");
			return(-1);
		}

		matrix4x4_vec3_mult(m_inv, lightdir_c, l_inv);

		fprintf(stderr, "l_inv = %f %f %f %f\n", l_inv[0], l_inv[1], l_inv[2], l_inv[3]);

		lightdir_parm_c->setValue(l_inv);
	}

	// Begin drawing
	
	render_action_c->beginDraw(VZ_RESTORE_GL_STATE_BIT);
//OGLCheckState();
	render_action_c->draw(shape_c);
	render_action_c->endDraw();
	glDisable(GL_BLEND);

	return(0);
}

vzParameterVolumeTexture	*DVRVolumizer::compute_gradient_volume(
	unsigned char *scalars
) {

	int	size;
	vzParameterVolumeTexture* texture;
	vzExternalTextureFormat dataFormat;
	vzTextureType dataType;
	//Gradient3D	*gradient3d;

	dataFormat = volume_c->getDataFormat();
	if(dataFormat != VZ_LUMINANCE) {
		QString strng("Volumizer Error; unsupported data format: ");
		strng += QString::number(dataFormat);
		Params::BailOut(strng.ascii(), __FILE__, __LINE__);
	}

	dataType = volume_c->getDataType ();
	if(dataType != VZ_UNSIGNED_BYTE) {
		QString strng("Volumizer Error; unsupported voxel type:");
		strng += QString::number(dataType);
		Params::BailOut(strng.ascii(), __FILE__, __LINE__);
	}

	size = data_dim_c[0] * data_dim_c[1] * data_dim_c[2] * 4;

	if (size > grad_data_size_c) {
		if (grad_data_c) delete [] grad_data_c;
		grad_data_c = new unsigned char[size];
		if (! grad_data_c) {
			MessageReporter::errorMsg(
			"Volumizer Error, gradient malloc error,\ninsufficient memory");
			return(NULL);
		}
		grad_data_size_c = size;
	}


#define	DEAD
#ifdef	DEAD
	

	computeGradientData(data_dim_c, 1, scalars, grad_data_c);
	fprintf(stderr, "slow gradient calc\n");

#else 

	gradient3d = new Gradient3D(
		data_dim_c, data_roi_c, 0, nthreads_c, NULL, NULL
	);
	if (gradient3d->GetErrCode() != 0) {
		//qWarning("Gradient3D : %s", gradient3d->GetErrMsg());
		return(NULL);
	}

	gradient3d->Compute(scalars, grad_data_c);

	delete gradient3d;

#endif

	texture = new vzParameterVolumeTexture(
		data_dim_c, data_roi_c, grad_data_c, VZ_BYTE, VZ_RGBA, VZ_INTENSITY8
	);
	texture->addDeletionCallback(deletion_cb,(void *) "vzParameterVolumeTexture 2");

	return(texture);
}


static void tex0(vzTMShaderData *data)
{
	// enable "volume" texture
	data->bindVolumeTextureCB("volume", data, VZ_TM_ENABLE);

	// bind "volume" texture
	if(!data->bindVolumeTextureCB("volume", data,VZ_TM_BIND))
		fprintf(stderr, "slice0: Error binding texture 'volume'\n");
}

static void tex1(vzTMShaderData *data)
{
	// enable "lookup_table" parameter (1D texture on Onyx4 systems)
	data->bindLookupTableCB("lookup_table", data, VZ_TM_ENABLE);

	// bind "lookup_table" parameter
	if(!data->bindLookupTableCB("lookup_table", data, VZ_TM_BIND))
		fprintf(stderr, "slice1: Error binding texture 'lookup_table'\n");

	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

vzAppearance	*DVRVolumizer::create_appearance()
{

	if (do_lighting_c) {
		//fprintf(stderr, "creating vzTMGradientShader\n");
		shader_c = new vzTMGradientShader();
		shader_c->addDeletionCallback(deletion_cb, (void *) "vzTMGradientShader");
	}
	else if (do_frag_prog_c) {
		static char FragmentProgram[] = "!!ARBfp1.0\n"
			"TEMP volume;\n"
			"TEX volume, fragment.texcoord[0], texture[0], 3D;\n"
			"TEX result.color, volume, texture[1], 1D;\n"
			"END";
		MessageReporter::infoMsg( "creating vzTMFragmentProgram");
		vzTMFragmentProgram *fp = new vzTMFragmentProgram(FragmentProgram);
		const int texCallbacks = 2;
		vzTMShaderCB *multiTexCB = new vzTMShaderCB[texCallbacks];

		multiTexCB[0] = tex0;
		multiTexCB[1] = tex1;
		fp->setMultiTexCallbacks(texCallbacks, multiTexCB, fp);

		shader_c = (vzShader*) fp;
		shader_c->addDeletionCallback(deletion_cb, (void *) "vzTMFragmentProgram");

	}
	else {
		MessageReporter::infoMsg( "creating vzTMLUTShader");
		shader_c = new vzTMLUTShader();
		shader_c->addDeletionCallback(deletion_cb, (void *) "vzTMLUTShader");
	}

	appearance_c = new vzAppearance(shader_c);
	shader_c->unref();

	if (do_lighting_c) {
		table_c = new vzParameterLookupTable(256, llut_c, VZ_FLOAT, VZ_RGBA);
	}
	else {
		table_c = new vzParameterLookupTable(256, lut_c, VZ_FLOAT, VZ_RGBA);
	}

	table_c->addDeletionCallback(deletion_cb, (void *) "vzParameterLookupTable");
	appearance_c->setParameter("lookup_table", table_c);
	table_c->unref();

	// only used with GradientShader, but doesn't seem to hurt anything
	// with lut shader
	//
    lightdir_parm_c = new vzParameterVec3f (lightdir_c);
	lightdir_parm_c->addDeletionCallback(deletion_cb, (void *) "vzParameterVec3f");
	appearance_c->setParameter("lightdir", lightdir_parm_c);
	lightdir_parm_c->unref();

	float ambient_vec[3] = {ambient_c[0], ambient_c[1], ambient_c[2]};
	ambient_parm_c = new vzParameterVec3f(ambient_vec);
	ambient_parm_c->addDeletionCallback(deletion_cb, (void *) "vzParameterVec3f");
	appearance_c->setParameter("ambient", ambient_parm_c);
	ambient_parm_c->unref();

	float diffuse_vec[3] = {diffuse_c[0], diffuse_c[1], diffuse_c[2]};
	diffuse_parm_c = new vzParameterVec3f(diffuse_vec);
	diffuse_parm_c->addDeletionCallback(deletion_cb, (void *) "vzParameterVec3f");
	appearance_c->setParameter("diffuse", diffuse_parm_c);
	diffuse_parm_c->unref();

	return(appearance_c);
}


#endif //VOLUMIZER


