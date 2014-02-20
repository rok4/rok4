/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <geop_services@geoportail.fr>
 *
 * This software is a computer program whose purpose is to publish geographic
 * data using OGC WMS and WMTS protocol.
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 *
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

/**
 * \file LibkakaduImage.cpp
 ** \~french
 * \brief Implémentation des classes LibkakaduImage et LibkakaduImageFactory
 * \details
 * \li LibkakaduImage : gestion d'une image au format JPEG2000, en lecture, utilisant la librairie openjpeg
 * \li LibkakaduImageFactory : usine de création d'objet LibkakaduImage
 ** \~english
 * \brief Implement classes LibkakaduImage and LibkakaduImageFactory
 * \details
 * \li LibkakaduImage : manage a JPEG2000 format image, reading, using the library openjpeg
 * \li LibkakaduImageFactory : factory to create LibkakaduImage object
 */


#include "LibkakaduImage.h"
#include "Logger.h"
#include "Utils.h"
#include "jp2.h"
#include "jpx.h"
#include "kdu_compressed.h"
#include "kdu_file_io.h"

/*****************************************************************************/
/* STATIC                   expand_single_threaded                           */
/*****************************************************************************/

static kdu_long
  expand_single_threaded(kdu_codestream codestream, kdu_dims tile_indices,
                         kde_file_binding *outputs, int num_output_channels,
                         bool last_output_channel_is_alpha,
                         bool alpha_is_premultiplied,
                         int num_used_components, int *used_component_indices,
                         jp2_channels channels, jp2_palette palette,
                         bool allow_shorts, bool skip_ycc,
                         int dwt_stripe_height,
                         int num_stats_layers, int num_stats_resolutions,
                         kdu_long *stats_reslayer_bytes,
                         kdu_long *stats_reslayer_packets,
                         kdu_long *stats_resmax_packets,
                         int progress_interval)
  /* This function wraps up the operations required to actually decompress
     the image samples.  It is called directly from `main' after setting up
     the output files (passed in via the `outputs' list), configuring the
     `codestream' object and parsing relevant command-line arguments.
        This particular function implements all decompression processing
     using a single thread of execution.  This is the simplest approach.
     From version 5.1 of Kakadu, the processing may also be efficiently
     distributed across multiple threads, which allows for the
     exploitation of multiple physical processors.  The implementation in
     that case is only slightly different from the multi-threaded case, but
     we encapsulate it in a separate version of this function,
     `expand_multi_threaded', mainly for illustrative purposes.
        The function returns the amount of memory allocated for sample
     processing, including all intermediate line buffers managed by
     the DWT engines associated with each active tile-component and the block
     decoding machinery associated with each tile-component-subband.
        The implementation here processes image lines one-by-one,
     maintaining W complete tile processing engines, where W is the number
     of tiles which span the width of the image (or the image region which
     is being reconstructed).  There are a variety of alternate processing
     paradigms which can be used.  The "kdu_buffered_expand" application
     demonstrates a different strategy, managed by the higher level
     `kdu_stripe_decompressor' object, in which whole image stripes are
     decompressed into a memory buffer.  If the stripe height is equal to
     the tile height, only one tile processing engine need be active at
     any given time in that model.  Yet another model is used by the
     `kdu_region_decompress' object, which decompresses a specified image
     region into a memory buffer.  In that case, processing is done
     tile-by-tile. */
{
  int x_tnum;
  kde_flow_control **tile_flows = new kde_flow_control *[tile_indices.size.x];
  for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
    {
      tile_flows[x_tnum] = new
        kde_flow_control(outputs,num_output_channels,
                         last_output_channel_is_alpha,alpha_is_premultiplied,
                         num_used_components,used_component_indices,
                         codestream,x_tnum,allow_shorts,channels,palette,
                         skip_ycc,dwt_stripe_height);
      if (num_stats_layers > 0)
        tile_flows[x_tnum]->collect_layer_stats(num_stats_layers,
                                                num_stats_resolutions,
                                                num_stats_resolutions-1,
                                                stats_reslayer_bytes,
                                                stats_reslayer_packets,
                                                stats_resmax_packets,NULL);
    }
  bool done = false;
  int tile_row = 0; // Just for progress counter
  int progress_counter = 0;
  while (!done)
    { 
      while (!done)
        { // Process a row of tiles line by line.
          done = true;
          for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
            {
              if (tile_flows[x_tnum]->advance_components())
                {
                  done = false;
                  tile_flows[x_tnum]->process_components();
                }
            }
          if ((!done) && ((++progress_counter) == progress_interval))
            { 
              pretty_cout << "\t\tProgress with current tile row = "
                          << tile_flows[0]->percent_pulled() << "%\n";
              progress_counter = 0;
            }
        }
      for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
        if (tile_flows[x_tnum]->advance_tile())
          done = false;
      tile_row++;
      progress_counter = 0;
      if (progress_interval > 0)
        pretty_cout << "\tFinished processing " << tile_row
                    << " of " << tile_indices.size.y << " tile rows\n";
    }
  kdu_long processing_sample_bytes = 0;
  for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
    {
      processing_sample_bytes += tile_flows[x_tnum]->get_buffer_memory();
      delete tile_flows[x_tnum];
    }
  delete[] tile_flows;

  return processing_sample_bytes;
}

/*****************************************************************************/
/* STATIC                 extract_jp2_resolution_info                        */
/*****************************************************************************/

static void
  extract_jp2_resolution_info(kdu_image_dims &idims, jp2_resolution resolution,
                              kdu_coords ref_size, double roi_width_fraction,
                              double roi_height_fraction, bool transpose,
                              bool dont_save_resolution)
  /* Extracts pixel resolution (and aspect ratio) information from the
     `resolution' interface and uses this to invoke `idims.set_resolution'
     so that resolution and associated information can be saved with the
     image.  Since the image might be rendered at a reduced resolution, or
     within a reduced region of interest, we send reference image dimensions
     to `idims.set_resolution', against which the display/capture resolution
     information should be assessed.  These are compared with the actual
     rendered image dimensions to determine how the pixel resolution
     information should be scaled -- done automatically by
     `idims.get_resolution'.  The reference image dimensions are taken from
     `ref_size', which is the full size of the relevant compositing layer
     (in its most general form, `jp2_resolution' is supposed to be interpreted
     relative to the compositing layer) and scaled by the `roi_..._fraction'
     parameters, which represent the fraction of the image width/height
     which remains after the region of interest has been extracted for
     saving to the output file.  These parameters are all expressed in the
     original coordinate image geometry (prior to any transposition), but
     the `idims' information needs to be configured for the final rendering
     geometry, which is why we need the `transpose' argument. */
{
  if ((ref_size.x <= 0) || (ref_size.y <= 0))
    return;

  // Start by seeing whether we have display resolution info, or only capture
  // resolution info.
  bool for_display=true, have_absolute_res=true;
  float ypels_per_metre = resolution.get_resolution(for_display);
  if (ypels_per_metre <= 0.0F)
    {
      for_display = false;
      ypels_per_metre = resolution.get_resolution(for_display);
      if (ypels_per_metre <= 0.0F)
        { have_absolute_res = false; ypels_per_metre = 1.0F; }
    }
  float xpels_per_metre =
    ypels_per_metre * resolution.get_aspect_ratio(for_display);
  assert(xpels_per_metre > 0.0F);
  double ref_width = ((double) ref_size.x) * roi_width_fraction;
  double ref_height = ((double) ref_size.y) * roi_height_fraction;
  if (transpose)
    idims.set_resolution(ref_height,ref_width,have_absolute_res,
                         ypels_per_metre,xpels_per_metre,dont_save_resolution);
  else
    idims.set_resolution(ref_width,ref_height,have_absolute_res,
                         xpels_per_metre,ypels_per_metre,dont_save_resolution);
}

/*****************************************************************************/
/* STATIC                   extract_jp2_colour_info                          */
/*****************************************************************************/

static void
  extract_jp2_colour_info(kdu_image_dims &idims, jp2_channels channels,
                          jp2_colour colour, bool have_alpha,
                          bool alpha_is_premultiplied)
{
  int num_colours = channels.get_num_colours();
  bool have_premultiplied_alpha = have_alpha && alpha_is_premultiplied;
  bool have_unassociated_alpha = have_alpha && !alpha_is_premultiplied;
  int colour_space_confidence = 1;
  jp2_colour_space space = colour.get_space();
  if ((space == JP2_iccLUM_SPACE) ||
      (space == JP2_iccRGB_SPACE) ||
      (space == JP2_iccANY_SPACE) ||
      (space == JP2_vendor_SPACE))
    colour_space_confidence = 0;
  idims.set_colour_info(num_colours,have_premultiplied_alpha,
                        have_unassociated_alpha,colour_space_confidence,
                        space);
}

/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

/* ----- Pour la lecture ----- */
LibkakaduImage* LibkakaduImageFactory::createLibkakaduImageToRead ( char* filename, BoundingBox< double > bbox, double resx, double resy ) {
    
    int width = 0, height = 0, bitspersample = 0, channels = 0;
    SampleFormat::eSampleFormat sf = SampleFormat::UNKNOWN;
    Photometric::ePhotometric ph = Photometric::UNKNOWN;
    
    /************** INITIALISATION DES OBJETS KAKADU *********/
    /*
    kdu_compressed_source *input = NULL;
    kdu_simple_file_source file_in;
    jp2_family_src jp2_ultimate_src;
    jpx_source jpx_in;
    
    jp2_ultimate_src.open(filename,true);
    kdu_coords layer_size; // Zero unless `jpx_layer' exists.  Records the size
    // of the layer on its registration.  According to the JPX standard,
    // this is the size against which any `jp2_resolution' information is
    // to be interpreted -- interpretation is backward compatible with JP2.
    if (jpx_in.open(&jp2_ultimate_src,true) < 0) { // Not compatible with JP2 or JPX.  Try opening as a raw code-stream.
        LOGGER_ERROR ( "Unhandled format for the JPEG2000 file " << filename );
        jp2_ultimate_src.close();
        return NULL;
    }
    
    input = jpx_stream.open_stream();
    
    // Create the codestream object.
    kdu_codestream codestream;
    codestream.create(input);*/

    // Open the input file in an appropriate way
    
    kdu_compressed_source *input = NULL;
    kdu_simple_file_source file_in;
    jp2_family_src jp2_ultimate_src;
    jpx_source jpx_in;
    jpx_codestream_source jpx_stream;
    jpx_layer_source jpx_layer;
    jp2_channels channels;
    jp2_palette palette;
    jp2_resolution resolution;
    jp2_colour colour;
    jp2_ultimate_src.open(input,true);
    kdu_coords layer_size = 0;
    
    bool transpose, vflip, hflip, allow_shorts, mem, stats, quiet;
    std::ostream *record_stream;
    float max_bpp;
    bool dont_save_resolution, want_comments, no_decode, no_seek;
    bool simulate_parsing, want_alpha;
    int jpx_layer_idx, raw_codestream, skip_components;
    int max_layers, discard_levels;
    int num_threads, double_buffering_height, progress_interval, cpu_iterations;
    int rotate;
    kdu_component_access_mode component_access_mode;

    record_stream = NULL;
    rotate = 0;
    max_bpp = -1.0F;
    allow_shorts = true;
    skip_components = 0;
    want_alpha = true;
    jpx_layer = 0;
    raw_codestream = -1;
    component_access_mode = KDU_WANT_OUTPUT_COMPONENTS;
    no_seek = false;
    max_layers = 0;
    discard_levels = 0;
    num_threads = 0; // This is not actually the default -- see below.
    double_buffering_height = 0; // i.e., no double buffering
    progress_interval = 0; // Don't provide any progress indication at all
    cpu_iterations = -1;
    simulate_parsing = false;
    mem = false;
    stats = false;
    dont_save_resolution = false;
    no_decode = false;
    quiet = false;
  
    /*if (jpx_in.open(&jp2_ultimate_src,true) < 0) { // Not compatible with JP2 or JPX.  Try opening as a raw code-stream.
        LOGGER_ERROR ( "Unhandled format for the JPEG2000 file " << filename );
        jp2_ultimate_src.close();
        return NULL;
    }*/
  
  if (jpx_in.open(&jp2_ultimate_src,true) < 0)
    { // Not compatible with JP2 or JPX.  Try opening as a raw code-stream.
      jp2_ultimate_src.close();
      file_in.open(input,!no_seek);
      input = &file_in;
      raw_codestream = 0;
    }
  else
    { // We have an open JP2/JPX-compatible file.
      if (skip_components && (raw_codestream < 0))
        { kdu_error e; e << "The `-skip_components' argument may be "
          "used only with raw code-stream sources or with the "
          "`-raw_components' switch."; }
      if (raw_codestream >= 0)
        {
          jpx_stream = jpx_in.access_codestream(raw_codestream);
          if (!jpx_stream)
            { kdu_error e; e << "Unable to find the code-stream identified "
              "by your `-raw_components' argument.  Note that the first "
              "code-stream in the file has an index of 0."; }
        }
      else
        {
          if (component_access_mode == KDU_WANT_CODESTREAM_COMPONENTS)
            { kdu_error e; e << "The `-codestream_components' flag may "
              "not be used with JP2/JPX sources unless you explicitly "
              "identify raw codestream access via the `-raw_components' "
              "argument."; }
          jpx_layer = jpx_in.access_layer(jpx_layer_idx);
          if (!jpx_layer)
            { kdu_error e; e << "Unable to find the compositing layer "
              "identified by your `-jpx_layer' argument.  Note that the "
              "first layer in the file has an index of 0."; }
          channels = jpx_layer.access_channels();
          resolution = jpx_layer.access_resolution();
          colour = jpx_layer.access_colour(0);
          layer_size = jpx_layer.get_layer_size();
          int cmp, plt, stream_id;
          channels.get_colour_mapping(0,cmp,plt,stream_id);
          jpx_stream = jpx_in.access_codestream(stream_id);
          palette = jpx_stream.access_palette();
        }
      input = jpx_stream.open_stream();
    }

  // Create the codestream object.
  kdu_codestream codestream;
  codestream.create(input);
  if (cpu_iterations >= 0)
    codestream.collect_timing_stats(cpu_iterations);

  kdu_message_formatter *formatted_recorder = NULL;
  kdu_stream_message recorder(record_stream);
  if (record_stream != NULL)
    {
      formatted_recorder = new kdu_message_formatter(&recorder);
      codestream.set_textualization(formatted_recorder);
    }
  kdu_dims region;
  double roi_width_fraction=1.0, roi_height_fraction=1.0;
  
  roi_width_fraction = roi_height_fraction = 1.0;
  if (!(codestream.access_siz()->get(Sorigin,0,0,region.pos.y) &&
        codestream.access_siz()->get(Sorigin,0,1,region.pos.x) &&
        codestream.access_siz()->get(Ssize,0,0,region.size.y) &&
        codestream.access_siz()->get(Ssize,0,1,region.size.x)))
    assert(0);
  region.size.y -= region.pos.y;
  region.size.x -= region.pos.x;
  
  codestream.apply_input_restrictions(skip_components,0,discard_levels,
                                      max_layers,&region,
                                      component_access_mode);
  codestream.change_appearance(transpose,vflip,hflip);
  
  kdu_image_dims idims;
  
  // Get the component (or mapped colour channel) dimensional properties
  int n, num_channels, num_components;
  num_components = codestream.get_num_components(true);
  if (channels.exists())
    num_channels = channels.get_num_colours();
  else
    num_channels = num_components;

  for (n=0; n < num_channels; n++)
    {
      kdu_dims dims;
      int precision;
      bool is_signed;

      int cmp=n, plt=-1, stream_id;
      if (channels.exists())
        {
          channels.get_colour_mapping(n,cmp,plt,stream_id);
          if (stream_id != jpx_stream.get_codestream_id())
            {
              num_channels = n; // Process only initial channels which come
                                // from the same code-stream.
              break;
            }
        }
      codestream.get_dims(cmp,dims,true);
      if (plt < 0)
        {
          precision = codestream.get_bit_depth(cmp,true);
          is_signed = codestream.get_signed(cmp,true);
        }
      else
        {
          precision = palette.get_bit_depth(plt);
          is_signed = palette.get_signed(plt);
        }
      idims.add_component(dims.size.y,dims.size.x,precision,is_signed);
    }

  // See if we can add an extra alpha channel to the collection; this is a
  // little involved, since there are two types of alpha components which can
  // be represented in JP2/JPX files.  I have included this stuff despite
  // my inclination to leave it out of a demo application, only because
  // so many people seem to judge what Kakadu can do based on its demonstration
  // applications.
  bool last_channel_is_alpha=false, alpha_is_premultiplied=false;
  int alpha_component_idx=-1; // Valid only if `last_channel_is_alpha'
  if (want_alpha && channels.exists())
    {
      int cmp=-1, plt=-1, stream_id;
      if (channels.get_opacity_mapping(0,cmp,plt,stream_id) &&
          (stream_id == jpx_stream.get_codestream_id()))
        {
          last_channel_is_alpha = true;
          alpha_is_premultiplied = false;
          alpha_component_idx = cmp;
        }
      else if (channels.get_premult_mapping(0,cmp,plt,stream_id) &&
               (stream_id == jpx_stream.get_codestream_id()))
        {
          last_channel_is_alpha = true;
          alpha_is_premultiplied = true;
          alpha_component_idx = cmp;
        }
      if (last_channel_is_alpha)
        {
          kdu_dims dims;
          int precision;
          bool is_signed;

          num_channels++;
          codestream.get_dims(cmp,dims,true);
          if (plt < 0)
            {
              precision = codestream.get_bit_depth(cmp,true);
              is_signed = codestream.get_signed(cmp,true);
            }
          else
            {
              precision = palette.get_bit_depth(plt);
              is_signed = palette.get_signed(plt);
            }
          idims.add_component(dims.size.y,dims.size.x,precision,is_signed);
        }
    }

  if (resolution.exists())
    extract_jp2_resolution_info(idims,resolution,layer_size,
                                roi_width_fraction,roi_height_fraction,
                                transpose,dont_save_resolution);
  if (channels.exists())
    extract_jp2_colour_info(idims,channels,colour,
                            last_channel_is_alpha,
                            alpha_is_premultiplied);
  if (jpx_in.exists())
    idims.set_meta_manager(jpx_in.access_meta_manager());

  // Now we are ready to open the output files.
  kde_file_binding *oscan;
  bool extra_vertical_flip = false;
  int num_output_channels=0;
  for (oscan=outputs; oscan != NULL; oscan=oscan->next)
    {
      bool flip;

      oscan->first_channel_idx = num_output_channels;
      oscan->writer =
        kdu_image_out(oscan->fname,idims,num_output_channels,flip,quiet);
      oscan->num_channels = num_output_channels - oscan->first_channel_idx;
      if (oscan == outputs)
        extra_vertical_flip = flip;
      if (extra_vertical_flip != flip)
        { kdu_error e; e << "Cannot mix output file types which have "
          "different vertical ordering conventions (i.e., top-to-bottom and "
          "bottom-to-top)."; }
    }
  if (extra_vertical_flip)
    {
      vflip = !vflip;
      codestream.change_appearance(transpose,vflip,hflip);
    }
  if (num_output_channels == 0)
    num_output_channels=num_channels; // No output files specified
  if (num_output_channels < num_channels)
    last_channel_is_alpha = false;


  // Find out which codestream components are actually used
  int num_used_components = 0;
  int *used_component_indices = new int[num_output_channels];
  for (n=0; n < num_output_channels; n++)
    {
      int c, cmp;

      if ((n==(num_channels-1)) && last_channel_is_alpha)
        cmp = alpha_component_idx;
      else if (channels.exists())
        {
          int plt, stream_id;
          channels.get_colour_mapping(n,cmp,plt,stream_id);
        }
      else
        cmp = skip_components+n;

      for (c=0; c < num_used_components; c++)
        if (used_component_indices[c] == cmp)
          break;
      if (c == num_used_components)
        used_component_indices[num_used_components++] = cmp;
    }

  // The following call saves us the cost of buffering up unused image
  // components.  All image components with indices other than those
  // recorded in `used_image_components' will be discarded.  However, we
  // have to remember from now on to use the indices of the relevant
  // component's entry in `used_component_indices' rather than the actual
  // component index, in calls into the `kdu_codestream' machinery.  This is
  // because the `apply_input_restrictions' function creates a new view of
  // the image in which the identified components are the only ones which
  // appear to be available.
  codestream.apply_input_restrictions(num_used_components,
                                      used_component_indices,discard_levels,
                                      max_layers,&region,
                                      component_access_mode);
  
  int num_stats_layers = 0;
  int num_stats_resolutions = 0;
  kdu_long *stats_reslayer_bytes = NULL;
  kdu_long *stats_reslayer_packets = NULL;
  kdu_long *stats_resmax_packets = NULL;
  if (stats)
    { // Set things up to collect parsed packet statistics
      num_stats_layers = 128;
      num_stats_resolutions = discard_levels+1;
      int num_stats_entries = num_stats_layers * num_stats_resolutions;
      stats_reslayer_bytes = new kdu_long[num_stats_entries];
      stats_reslayer_packets = new kdu_long[num_stats_entries];
      stats_resmax_packets = new kdu_long[num_stats_resolutions];
      memset(stats_reslayer_bytes,0,sizeof(kdu_long)*num_stats_entries);
      memset(stats_reslayer_packets,0,sizeof(kdu_long)*num_stats_entries);
      memset(stats_resmax_packets,0,sizeof(kdu_long)*num_stats_resolutions);
    }

  // Now we are ready for sample data processing.
  kdu_dims tile_indices; codestream.get_valid_tiles(tile_indices);
  kdu_long sample_processing_bytes=0;
  bool skip_ycc = (component_access_mode == KDU_WANT_CODESTREAM_COMPONENTS);

  int dwt_stripe_height = 1;
  if (double_buffering_height > 0)
    dwt_stripe_height = double_buffering_height;
  sample_processing_bytes =
    expand_single_threaded(codestream,tile_indices,outputs,
                           num_output_channels,
                           last_channel_is_alpha,alpha_is_premultiplied,
                           num_used_components,used_component_indices,
                           channels,palette,allow_shorts,skip_ycc,
                           dwt_stripe_height,
                           num_stats_layers,num_stats_resolutions,
                           stats_reslayer_bytes,stats_reslayer_packets,
                           stats_resmax_packets,progress_interval);
   
  // Cleanup
  if ((cpu_iterations >= 0) && !no_decode)
    {
      kdu_long num_samples;
      double seconds = codestream.get_timing_stats(&num_samples);
      pretty_cout << "End-to-end CPU time ";
      if (cpu_iterations > 0)
        pretty_cout << "(estimated) ";
      pretty_cout << "= " << seconds << " seconds ("
                  << 1.0E6*seconds/num_samples << " us/sample)\n";
    }
  if ((cpu_iterations > 0) && !no_decode)
    {
      kdu_long num_samples;
      double seconds = codestream.get_timing_stats(&num_samples,true);
      if (seconds > 0.0)
        {
          pretty_cout << "Block decoding CPU time (estimated) ";
          pretty_cout << "= " << seconds << " seconds ("
                      << 1.0E6*seconds/num_samples << " us/sample)\n";
        }
    }
  if (mem)
    {
      pretty_cout << "\nSample processing/buffering memory = "
                  << sample_processing_bytes << " bytes.\n";
      pretty_cout << "Compressed data memory = "
                  << codestream.get_compressed_data_memory()
                  << " bytes.\n";
      pretty_cout << "State memory associated with compressed data = "
                  << codestream.get_compressed_state_memory()
                  << " bytes.\n";
    }
  if (!quiet)
    {
      pretty_cout << "\nConsumed " << codestream.get_num_tparts()
                  << " tile-part(s) from a total of "
                  << tile_indices.area() << " tile(s).\n";
      pretty_cout << "Consumed "
                  << codestream.get_total_bytes()
                  << " codestream bytes (excluding any file format) = "
                  << 8.0*codestream.get_total_bytes() /
                     get_bpp_dims(codestream.access_siz())
                  << " bits/pel.\n";
      if ((!no_decode) &&
          jpx_in.exists() && !jpx_in.access_compatibility().is_jp2())
        {
          if (jpx_layer.exists())
            pretty_cout << "Decompressed channels are based on JPX "
                           "compositing layer "
                        << jpx_layer.get_layer_id() << ".\n";
          if (jpx_stream.exists())
            pretty_cout << "Decompressed codestream "
                        << jpx_stream.get_codestream_id()
                        << " of JPX file.\n";
        }
      if ((num_threads == 0) && !no_decode)
        pretty_cout << "Processed using the single-threaded environment "
        "(see `-num_threads')\n";
      else if (!no_decode)
        pretty_cout << "Processed using the multi-threaded environment, with\n"
          "    " << num_threads << " parallel threads of execution "
          "(see `-num_threads')\n";
    }
  delete[] used_component_indices;
  if (stats)
    {
      print_parsed_packet_stats(num_stats_layers,num_stats_resolutions,
                                stats_reslayer_bytes,stats_reslayer_packets,
                                stats_resmax_packets);
      delete[] stats_reslayer_bytes;
      delete[] stats_reslayer_packets;
      delete[] stats_resmax_packets;
    }
  codestream.destroy();
  input->close();
  jpx_in.close();
  if (record_stream != NULL)
    {
      delete formatted_recorder;
      delete record_stream;
    }
  delete outputs;

    /********************** CONTROLES **************************/

    if ( ! SampleFormat::isHandledSampleType ( sf, bitspersample ) ) {
        LOGGER_ERROR ( "Not supported sample type : " << SampleFormat::toString ( sf ) << " and " << bitspersample << " bits per sample" );
        return NULL;
    }

    if ( resx > 0 && resy > 0 ) {
        // Vérification de la cohérence entre les résolutions et bbox fournies et les dimensions (en pixel) de l'image
        // Arrondi a la valeur entiere la plus proche
        int calcWidth = lround ( ( bbox.xmax - bbox.xmin ) / ( resx ) );
        int calcHeight = lround ( ( bbox.ymax - bbox.ymin ) / ( resy ) );
        if ( calcWidth != width || calcHeight != height ) {
            LOGGER_ERROR ( "Resolutions, bounding box and real dimensions for image '" << filename << "' are not consistent" );
            LOGGER_ERROR ( "Height is " << height << " and calculation give " << calcHeight );
            LOGGER_ERROR ( "Width is " << width << " and calculation give " << calcWidth );
            return NULL;
        }
    }

    return new LibkakaduImage (
        width, height, resx, resy, channels, bbox, filename,
        sf, bitspersample, ph, Compression::JPEG2000
    );

}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

LibkakaduImage::LibkakaduImage (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, char* name,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric, Compression::eCompression compression) :

    Jpeg2000Image ( width, height, resx, resy, channels, bbox, name, sampleformat, bitspersample, photometric, compression )/*,

    jp2image ( jp2ptr )*/ {

}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

int LibkakaduImage::getline ( uint8_t* buffer, int line ) {
    
    //return width*channels;
    return -1;
}

int LibkakaduImage::getline ( float* buffer, int line ) {

    // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers (forcément pour du JPEG2000)
    uint8_t* buffer_t = new uint8_t[width*channels];
    int retour = getline ( buffer_t,line );
    convert ( buffer,buffer_t,width*channels );
    delete [] buffer_t;
    return retour;
}

