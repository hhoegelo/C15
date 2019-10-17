package com.nonlinearlabs.NonMaps.client.world.overlay.belt.sound;

import com.google.gwt.canvas.dom.client.Context2d;
import com.nonlinearlabs.NonMaps.client.Millimeter;
import com.nonlinearlabs.NonMaps.client.dataModel.editBuffer.EditBufferModel;
import com.nonlinearlabs.NonMaps.client.dataModel.editBuffer.EditBufferModel.VoiceGroup;
import com.nonlinearlabs.NonMaps.client.world.Gray;
import com.nonlinearlabs.NonMaps.client.world.Rect;
import com.nonlinearlabs.NonMaps.client.world.overlay.Label;
import com.nonlinearlabs.NonMaps.client.world.overlay.OverlayLayout;

public class LayerSoundLayout extends SoundLayout {

	protected LayerSoundLayout(OverlayLayout parent) {
		super(parent);
		setSettings(new LayerSoundSettings(this));
	}

	private class LayerSoundSettings extends OverlayLayout {
		LayerSoundSettings(LayerSoundLayout parent) {
			super(parent);
			addChild(new VoiceGroupSoundSettings(VoiceGroup.I, this));
			addChild(new VoiceGroupSoundSettings(VoiceGroup.II, this));
		}

		@Override
		public void doLayout(double x, double y, double w, double h) {
			super.doLayout(x, y, w, h);
			double parts = 20;
			double unit = h / parts;
			getChildren().get(0).doLayout(0, 1 * unit, w, 8 * unit);
			getChildren().get(1).doLayout(0, 11 * unit, w, 8 * unit);
		}
	}

	private class VoiceGroupSoundSettings extends OverlayLayout {
		VoiceGroup group;

		VoiceGroupSoundSettings(VoiceGroup g, LayerSoundSettings parent) {
			super(parent);
			group = g;
			addChild(new VoiceGroupLabel(this));
			addChild(new PresetName(this));
			addChild(new TuneReference(this));
			addChild(new Volume(this));
		}

		@Override
		public void doLayout(double x, double y, double w, double h) {
			super.doLayout(x, y, w, h);
			double margin = Millimeter.toPixels(3);
			double parts = 20;
			double unit = (w - 2 * margin) / parts;

			getChildren().get(0).doLayout(margin + 0 * unit + margin, margin, 1 * unit - 2 * margin, h - 2 * margin);
			getChildren().get(1).doLayout(margin + 1 * unit + margin, margin, 9 * unit - 2 * margin, h - 2 * margin);
			getChildren().get(2).doLayout(margin + 10 * unit + margin, margin, 5 * unit - 2 * margin, h - 2 * margin);
			getChildren().get(3).doLayout(margin + 15 * unit + margin, margin, 5 * unit - 2 * margin, h - 2 * margin);
		}

		@Override
		public void draw(Context2d ctx, int invalidationMask) {
			double margin = Millimeter.toPixels(1);
			getPixRect().drawRoundedArea(ctx, margin, 1, new Gray(100), new Gray(100));

			double contentLeft = getChildren().get(1).getPixRect().getLeft();
			double contentRight = getPixRect().getRight();
			Rect contentRect = new Rect(contentLeft - 2 * margin, getPixRect().getTop() + 1 * margin,
					contentRight - contentLeft + 1 * margin, getPixRect().getHeight() - 2 * margin);
			contentRect.drawRoundedArea(ctx, margin, 1, new Gray(30), new Gray(30));
			super.draw(ctx, invalidationMask);
		}

		private class VoiceGroupLabel extends Label {

			VoiceGroupLabel(VoiceGroupSoundSettings parent) {
				super(parent);
			}

			@Override
			public String getDrawText(Context2d ctx) {
				return group == VoiceGroup.I ? "I" : "II";
			}

			@Override
			protected double getFontHeight(Rect pixRect) {
				return pixRect.getHeight();
			}

		}

		private class PresetName extends Label {
			PresetName(VoiceGroupSoundSettings parent) {
				super(parent);
			}

			@Override
			public String getDrawText(Context2d ctx) {
				return EditBufferModel.get().getPresetNameOfVoiceGroup(group);
			}

			@Override
			public void draw(Context2d ctx, int invalidationMask) {
				getPixRect().drawRoundedArea(ctx, Millimeter.toPixels(0.5), 1, new Gray(68), new Gray(86));
				super.draw(ctx, invalidationMask);
			}
		}

		private class TuneReference extends ValueEdit {
			TuneReference(VoiceGroupSoundSettings parent) {
				super(parent, EditBufferModel.get().findParameter(248));
			}
		}

		private class Volume extends ValueEdit {
			Volume(VoiceGroupSoundSettings parent) {
				super(parent, EditBufferModel.get().findParameter(248));
			}
		}
	}

}