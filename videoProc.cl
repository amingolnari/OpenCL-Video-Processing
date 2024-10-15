
__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | 
      CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST; 

__kernel void videoProc(read_only image2d_t src_image,
                        write_only image2d_t dst_image) {
					    int2 coord = (int2)(get_global_id(0), get_global_id(1));
					    int2 startcoord = (int2)(get_global_id(0)-1, get_global_id(1)-1);
					    int2 endcoord = (int2)(get_global_id(0)+1, get_global_id(1)+1);
						float sobx[9] = {1, 0, -1, 2, 0, -2, 1, 0, -1};
						float soby[9] = {1, 2, 1, 0, 0, 0, -1, -2, -1};
						int cunt = 0;
						float4 pix = {0.0f, 0.0f, 0.0f, 0.0f};
						float4 pix1 = {0.0f, 0.0f, 0.0f, 0.0f};
						float4 pix2 = {0.0f, 0.0f, 0.0f, 0.0f};
						for(int y = startcoord.s0; y <= endcoord.s0; y++){
							for(int x = startcoord.s1; x <= endcoord.s1; x++){
								pix1 += (read_imagef(src_image, sampler, (int2)(y, x)) * sobx[cunt]);
								pix2 += (read_imagef(src_image, sampler, (int2)(y, x)) * soby[cunt]);
								//printf("soby[%d] : %f ", cunt, soby[cunt]);
								//printf("sobx[%d] : %f ", cunt, sobx[cunt]);
								cunt++;
							}
						}
						//printf("%d - %d", W , H);
						pix = fmax(pix1, pix2);
						write_imagef(dst_image, coord, pix);
}