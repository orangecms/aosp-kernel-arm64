/*
 * drivers/amlogic/drm/meson_crtc.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include "meson_crtc.h"
#include "meson_vpu_pipeline.h"

#define OSD_DUMP_PATH		"/tmp/osd_dump/"

static int meson_crtc_set_mode(struct drm_mode_set *set)
{
	struct am_meson_crtc *amcrtc;
	int ret;

	DRM_DEBUG_DRIVER("%s\n", __func__);
	amcrtc = to_am_meson_crtc(set->crtc);
	ret = drm_atomic_helper_set_config(set);

	return ret;
}

static void meson_crtc_destroy_state(struct drm_crtc *crtc,
				     struct drm_crtc_state *state)
{
	struct am_meson_crtc_state *meson_crtc_state;

	meson_crtc_state = to_am_meson_crtc_state(state);
	__drm_atomic_helper_crtc_destroy_state(&meson_crtc_state->base);
	kfree(meson_crtc_state);
}

static struct drm_crtc_state *meson_crtc_duplicate_state(struct drm_crtc *crtc)
{
	struct am_meson_crtc_state *meson_crtc_state, *old_crtc_state;

	old_crtc_state = to_am_meson_crtc_state(crtc->state);

	meson_crtc_state = kmemdup(old_crtc_state, sizeof(*old_crtc_state),
				   GFP_KERNEL);
	if (!meson_crtc_state)
		return NULL;

	__drm_atomic_helper_crtc_duplicate_state(crtc, &meson_crtc_state->base);
	return &meson_crtc_state->base;
}

static int meson_crtc_atomic_get_property(struct drm_crtc *crtc,
					  const struct drm_crtc_state *state,
					struct drm_property *property,
					uint64_t *val)
{
	struct am_meson_crtc *amcrtc;
	struct am_meson_crtc_state *meson_crtc_state;
	int ret = -EINVAL;

	amcrtc = to_am_meson_crtc(crtc);
	meson_crtc_state = to_am_meson_crtc_state(state);
	if (property == amcrtc->prop_hdr_policy) {
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM
		meson_crtc_state->hdr_policy = get_hdr_policy();
#endif
		*val = meson_crtc_state->hdr_policy;
		ret = 0;
	} else if (property == amcrtc->prop_dv_policy) {
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		meson_crtc_state->dv_policy = get_dolby_vision_policy();
#endif
		*val = meson_crtc_state->dv_policy;
		ret = 0;
	} else if (property == amcrtc->prop_video_out_fence_ptr) {
		*val = 0;
		ret = 0;
	} else {
		DRM_INFO("unsupported crtc property\n");
	}
	return ret;
}

static int meson_crtc_atomic_set_property(struct drm_crtc *crtc,
					  struct drm_crtc_state *state,
					struct drm_property *property,
					uint64_t val)
{
	struct am_meson_crtc *amcrtc;
	struct am_meson_crtc_state *meson_crtc_state;
	s32 __user *fence_ptr;
	int ret = -EINVAL;

	amcrtc = to_am_meson_crtc(crtc);
	meson_crtc_state = to_am_meson_crtc_state(state);
	if (property == amcrtc->prop_hdr_policy) {
		meson_crtc_state->hdr_policy = val;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM
		set_hdr_policy(val);
#endif
		ret = 0;
	} else if (property == amcrtc->prop_dv_policy) {
		meson_crtc_state->dv_policy = val;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		set_dolby_vision_policy(val);
#endif
		ret = 0;
	} else if (property == amcrtc->prop_video_out_fence_ptr) {
		fence_ptr = u64_to_user_ptr(val);
		if (!fence_ptr)
			return -EFAULT;
		if (put_user(-1, fence_ptr))
			return -EFAULT;
		ret = 0;
		meson_crtc_state->fence_state.video_out_fence_ptr = fence_ptr;
		DRM_DEBUG("set video out fence ptr done\n");
	} else {
		DRM_INFO("unsupported crtc property\n");
	}
	return ret;
}

static const struct drm_crtc_funcs am_meson_crtc_funcs = {
	.atomic_destroy_state	= meson_crtc_destroy_state,
	.atomic_duplicate_state = meson_crtc_duplicate_state,
	.destroy		= drm_crtc_cleanup,
	.page_flip		= drm_atomic_helper_page_flip,
	.reset			= drm_atomic_helper_crtc_reset,
	.set_config             = meson_crtc_set_mode,
	.atomic_get_property = meson_crtc_atomic_get_property,
	.atomic_set_property = meson_crtc_atomic_set_property,
};

static bool am_meson_crtc_mode_fixup(struct drm_crtc *crtc,
				     const struct drm_display_mode *mode,
				struct drm_display_mode *adj_mode)
{
	//DRM_INFO("%s !!\n", __func__);

	return true;
}

static void am_meson_crtc_enable(struct drm_crtc *crtc)
{
	char *name;
	enum vmode_e mode;
	struct drm_display_mode *adjusted_mode = &crtc->state->adjusted_mode;
	struct am_meson_crtc *amcrtc = to_am_meson_crtc(crtc);
	struct meson_vpu_pipeline *pipeline = amcrtc->pipeline;

	DRM_INFO("%s\n", __func__);
	if (!adjusted_mode) {
		DRM_ERROR("meson_crtc_enable fail, unsupport mode:%s\n",
			  adjusted_mode->name);
		return;
	}
	DRM_INFO("%s: %s\n", __func__, adjusted_mode->name);

	name = am_meson_crtc_get_voutmode(adjusted_mode);
	mode = validate_vmode(name, 0);
	if (mode == VMODE_MAX) {
		DRM_ERROR("no matched vout mode\n");
		return;
	}
	set_vout_init(mode);
	update_vout_viu();
	memcpy(&pipeline->mode, adjusted_mode,
	       sizeof(struct drm_display_mode));
	enable_irq(amcrtc->irq);
}

static void am_meson_crtc_disable(struct drm_crtc *crtc)
{
	struct am_meson_crtc *amcrtc = to_am_meson_crtc(crtc);
	enum vmode_e mode;

	DRM_INFO("%s\n", __func__);
	if (crtc->state->event && !crtc->state->active) {
		spin_lock_irq(&crtc->dev->event_lock);
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		spin_unlock_irq(&crtc->dev->event_lock);
		crtc->state->event = NULL;
	}

	disable_irq(amcrtc->irq);
	/*disable output by config null
	 *Todo: replace or delete it if have new method
	 */
	mode = validate_vmode("null", 0);
	if (mode == VMODE_MAX) {
		DRM_ERROR("no matched vout mode\n");
		return;
	}
	set_vout_init(mode);
}

static void am_meson_crtc_commit(struct drm_crtc *crtc)
{
	//DRM_INFO("%s\n", __func__);
}

static int am_meson_atomic_check(struct drm_crtc *crtc,
				 struct drm_crtc_state *crtc_state)
{
	struct am_meson_crtc *amcrtc;
	struct meson_vpu_pipeline *pipeline;
	struct drm_atomic_state *state = crtc_state->state;

	amcrtc = to_am_meson_crtc(crtc);
	pipeline = amcrtc->pipeline;

	return vpu_pipeline_check(pipeline, state);
}

static int am_meson_video_fence_setup(struct video_out_fence_state *fence_state,
				      struct fence *fence)
{
	fence_state->fd = get_unused_fd_flags(O_CLOEXEC);
	if (fence_state->fd < 0)
		return fence_state->fd;

	if (put_user(fence_state->fd, fence_state->video_out_fence_ptr))
		return -EFAULT;

	fence_state->sync_file = sync_file_create(fence);
	if (!fence_state->sync_file)
		return -ENOMEM;

	return 0;
}

static int am_meson_video_fence_create(struct drm_crtc *crtc)
{
	struct am_meson_crtc *amcrtc;
	struct am_meson_crtc_state *meson_crtc_state;
	struct fence *fence;
	struct video_out_fence_state *fence_state;
	int ret, i;

	amcrtc = to_am_meson_crtc(crtc);
	meson_crtc_state = to_am_meson_crtc_state(crtc->state);
	fence_state = &meson_crtc_state->fence_state;
	fence = drm_crtc_create_fence(crtc);
	if (!fence)
		return -ENOMEM;
	/*setup out fence*/
	ret = am_meson_video_fence_setup(fence_state, fence);
	if (ret) {
		fence_put(fence);
		return ret;
	}
	fd_install(fence_state->fd, fence_state->sync_file->file);
	for (i = 0; i < VIDEO_LATENCY_VSYNC; i++) {
		if (amcrtc->video_fence[i].fence)
			continue;
		amcrtc->video_fence[i].fence = fence;
		atomic_set(&amcrtc->video_fence[i].refcount,
			   VIDEO_LATENCY_VSYNC);
		DRM_DEBUG("video fence create done index:%d\n", i);
		break;
	}
	return 0;
}

static void am_meson_crtc_atomic_begin(struct drm_crtc *crtc,
				       struct drm_crtc_state *old_crtc_state)
{
	struct am_meson_crtc *amcrtc;
	struct am_meson_crtc_state *meson_crtc_state;
	unsigned long flags;
	int ret = 0;

	amcrtc = to_am_meson_crtc(crtc);
	meson_crtc_state = to_am_meson_crtc_state(crtc->state);
	if (meson_crtc_state->fence_state.video_out_fence_ptr) {
		ret = am_meson_video_fence_create(crtc);
		meson_crtc_state->fence_state.video_out_fence_ptr = NULL;
	}
	if (ret)
		DRM_INFO("video fence create fail\n");

	if (crtc->state->event) {
		WARN_ON(drm_crtc_vblank_get(crtc) != 0);

		spin_lock_irqsave(&crtc->dev->event_lock, flags);
		amcrtc->event = crtc->state->event;
		spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
		crtc->state->event = NULL;
	}
}

static void am_meson_crtc_atomic_flush(struct drm_crtc *crtc,
				       struct drm_crtc_state *old_state)
{
	struct drm_color_ctm *ctm;
	struct drm_color_lut *lut;
	struct am_meson_crtc *amcrtc = to_am_meson_crtc(crtc);
	struct drm_atomic_state *old_atomic_state = old_state->state;
	struct meson_drm *priv = amcrtc->priv;
	struct meson_vpu_pipeline *pipeline = amcrtc->pipeline;
	#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT
	int gamma_lut_size = 0;
	#endif

	if (crtc->state->color_mgmt_changed) {
		DRM_INFO("%s color_mgmt_changed!\n", __func__);
		if (crtc->state->ctm) {
			DRM_INFO("%s color_mgmt_changed 1!\n", __func__);
			ctm = (struct drm_color_ctm *)
				crtc->state->ctm->data;
			#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT
			am_meson_ctm_set(0, ctm);
			#endif
		} else {
			DRM_DEBUG("%s Disable CTM!\n", __func__);
			#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT
			am_meson_ctm_disable();
			#endif
		}
	}
	if (crtc->state->gamma_lut != priv->gamma_lut_blob) {
		DRM_DEBUG("%s GAMMA LUT blob changed!\n", __func__);
		drm_property_unreference_blob(priv->gamma_lut_blob);
		priv->gamma_lut_blob = NULL;
		if (crtc->state->gamma_lut) {
			DRM_INFO("%s Set GAMMA\n", __func__);
			priv->gamma_lut_blob = drm_property_reference_blob(
				crtc->state->gamma_lut);
			lut = (struct drm_color_lut *)
				crtc->state->gamma_lut->data;
			#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT
			gamma_lut_size = amvecm_drm_get_gamma_size(0);
			amvecm_drm_gamma_set(0, lut, gamma_lut_size);
			#endif
		} else {
			DRM_DEBUG("%s Disable GAMMA!\n", __func__);
			#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT
			amvecm_drm_gamma_disable(0);
			#endif
		}
	}
#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
	meson_vpu_line_check(crtc->index, crtc->mode.vdisplay);
#endif
	vpu_pipeline_update(pipeline, old_atomic_state);
#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
	meson_vpu_reg_vsync_config();
#endif
}

static const struct drm_crtc_helper_funcs am_crtc_helper_funcs = {
	.enable			= am_meson_crtc_enable,
	.disable			= am_meson_crtc_disable,
	.commit			= am_meson_crtc_commit,
	.mode_fixup		= am_meson_crtc_mode_fixup,
	.atomic_check	= am_meson_atomic_check,
	.atomic_begin	= am_meson_crtc_atomic_begin,
	.atomic_flush		= am_meson_crtc_atomic_flush,
};

/* Optional hdr_policy properties. */
static const struct drm_prop_enum_list drm_hdr_policy_enum_list[] = {
	{ DRM_MODE_HDR_FOLLOW_SINK, "HDR_follow_sink" },
	{ DRM_MODE_HDR_FOLLOW_SOURCE, "HDR_follow_source" },
};

/* Optional dv_policy properties. */
static const struct drm_prop_enum_list drm_dv_policy_enum_list[] = {
	{ DRM_MODE_DV_FOLLOW_SINK, "DV_follow_sink" },
	{ DRM_MODE_DV_FOLLOW_SOURCE, "DV_follow_source" },
};

int drm_plane_create_hdr_policy_property(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	struct drm_property *prop;
	struct am_meson_crtc *amcrtc;
	int hdr_policy;

	#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM
	hdr_policy = get_hdr_policy();
	#else
	hdr_policy = DRM_MODE_HDR_FOLLOW_SINK
	#endif

	amcrtc = to_am_meson_crtc(crtc);
	prop = drm_property_create_enum(dev, 0, "hdr_policy",
					drm_hdr_policy_enum_list,
					ARRAY_SIZE(drm_hdr_policy_enum_list));
	if (!prop)
		return -ENOMEM;

	drm_object_attach_property(&crtc->base, prop, hdr_policy);
	amcrtc->prop_hdr_policy = prop;

	return 0;
}

int drm_plane_create_dv_policy_property(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	struct drm_property *prop;
	struct am_meson_crtc *amcrtc;
	int dv_policy;

	#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	dv_policy = get_dolby_vision_policy();
	#else
	dv_policy = DRM_MODE_DV_FOLLOW_SINK
	#endif

	amcrtc = to_am_meson_crtc(crtc);
	prop = drm_property_create_enum(dev, 0, "dv_policy",
					drm_dv_policy_enum_list,
					ARRAY_SIZE(drm_dv_policy_enum_list));
	if (!prop)
		return -ENOMEM;

	drm_object_attach_property(&crtc->base, prop, dv_policy);
	amcrtc->prop_dv_policy = prop;

	return 0;
}

int am_meson_crtc_create_video_out_fence_property(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	struct drm_property *prop;
	struct am_meson_crtc *amcrtc;

	amcrtc = to_am_meson_crtc(crtc);
	prop = drm_property_create_range(dev, DRM_MODE_PROP_ATOMIC,
					 "VIDEO_OUT_FENCE_PTR", 0, U64_MAX);
	if (!prop)
		return -ENOMEM;

	drm_object_attach_property(&crtc->base, prop, 0);
	amcrtc->prop_video_out_fence_ptr = prop;

	return 0;
}

int am_meson_crtc_create(struct am_meson_crtc *amcrtc)
{
	struct meson_drm *priv = amcrtc->priv;
	struct drm_crtc *crtc = &amcrtc->base;
	struct meson_vpu_pipeline *pipeline = priv->pipeline;
	#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT
	int gamma_lut_size = 0;
	#endif
	int ret;

	DRM_INFO("%s\n", __func__);
	ret = drm_crtc_init_with_planes(priv->drm, crtc,
					priv->primary_plane, priv->cursor_plane,
					&am_meson_crtc_funcs, "amlogic vpu");
	if (ret) {
		dev_err(amcrtc->dev, "Failed to init CRTC\n");
		return ret;
	}

	drm_plane_create_hdr_policy_property(crtc);
	drm_plane_create_dv_policy_property(crtc);
	am_meson_crtc_create_video_out_fence_property(crtc);
	drm_crtc_helper_add(crtc, &am_crtc_helper_funcs);
#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
	meson_vpu_reg_handle_register();
#endif
	#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT
	amvecm_drm_init(0);
	gamma_lut_size = amvecm_drm_get_gamma_size(0);
	drm_mode_crtc_set_gamma_size(crtc, gamma_lut_size);
	drm_crtc_enable_color_mgmt(crtc, 0, true, gamma_lut_size);
	#endif

	amcrtc->pipeline = pipeline;
	pipeline->crtc = crtc;
	strcpy(amcrtc->osddump_path, OSD_DUMP_PATH);
	priv->crtc = crtc;
	priv->crtcs[priv->num_crtcs++] = amcrtc;

	return 0;
}

