package com.nonlinearlabs.NonMaps.client.world.overlay.belt.presets;

import com.nonlinearlabs.NonMaps.client.Millimeter;
import com.nonlinearlabs.NonMaps.client.world.overlay.OverlayLayout;

public class LoadButtonArea extends OverlayLayout {

	private PrevNextPresetButtons prevNext;
	private LoadPreset load;
	AdvancedBankInformation bankInfos;

	protected LoadButtonArea(BeltPresetLayout parent) {
		super(parent);

		addChild(bankInfos = new AdvancedBankInformation(this));
		addChild(prevNext = new PrevNextPresetButtons(this));
		addChild(load = new LoadPreset(this));
	}

	public void doLayout(double right, double y, double h) {
		double buttonWidth = load.getSelectedImage().getImgWidth();
		double buttonHeight = load.getSelectedImage().getImgHeight();
		double margin = getMargin();
		double w = 2 * buttonWidth + margin;

		bankInfos.doLayout(0, 0, buttonWidth, Millimeter.toPixels(12));
		prevNext.doLayout(0, Millimeter.toPixels(12), buttonWidth, h - Millimeter.toPixels(12));
		load.doLayout(prevNext.getRelativePosition().getRight() + margin, Millimeter.toPixels(18), buttonWidth,
				buttonHeight);

		super.doLayout(right - w, y, w, h);
	}

	public static double getMargin() {
		return Millimeter.toPixels(2.5);
	}

	public PrevNextPresetButtons getPrevNext() {
		return prevNext;
	}

}
