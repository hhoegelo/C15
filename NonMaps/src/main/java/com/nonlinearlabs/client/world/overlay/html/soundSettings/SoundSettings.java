package com.nonlinearlabs.client.world.overlay.html.soundSettings;

import com.google.gwt.core.client.GWT;
import com.google.gwt.dom.client.InputElement;
import com.google.gwt.uibinder.client.UiBinder;
import com.google.gwt.uibinder.client.UiField;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.Composite;
import com.google.gwt.user.client.ui.HTMLPanel;
import com.google.gwt.user.client.ui.Label;
import com.nonlinearlabs.client.presenters.DeviceSettingsProvider;
import com.nonlinearlabs.client.useCases.EditBufferUseCases;
import com.nonlinearlabs.client.useCases.SystemSettings;
import com.nonlinearlabs.client.world.overlay.html.Range;

public class SoundSettings extends Composite {
	interface SoundSettingsUiBinder extends UiBinder<HTMLPanel, SoundSettings> {
	}

	private static SoundSettingsUiBinder ourUiBinder = GWT.create(SoundSettingsUiBinder.class);

	@UiField
	Button initialize, applyRandomization;

	@UiField
	Label randomValueDisplayString, transitionTimeDisplayString, tuneReferenceDisplayString;

	@UiField
	InputElement randomValueSlider, transitionTimeSlider, tuneReferenceSlider;

	Range randomValueSliderRange, transitionTimeSliderRange, tuneReferenceSliderRange;

	public SoundSettings() {
		initWidget(ourUiBinder.createAndBindUi(this));

		randomValueSliderRange = Range.wrap(randomValueSlider);
		transitionTimeSliderRange = Range.wrap(transitionTimeSlider);
		tuneReferenceSliderRange = Range.wrap(tuneReferenceSlider);

		connectEventHandlers();
		connectUpdate();

	}

	public void connectEventHandlers() {
		initialize.addClickHandler(e -> EditBufferUseCases.get().initializeSound());
		applyRandomization.addClickHandler(e -> EditBufferUseCases.get().randomizeSound());
		randomValueSliderRange
				.addValueChangeHandler(v -> SystemSettings.get().setRandomizeAmount(v.getValue().doubleValue()));
		transitionTimeSliderRange
				.addValueChangeHandler(v -> SystemSettings.get().setTransitionTime(v.getValue().doubleValue()));
		tuneReferenceSliderRange
				.addValueChangeHandler(v -> SystemSettings.get().setTuneReference(v.getValue().doubleValue()));
	}

	public void connectUpdate() {
		DeviceSettingsProvider.get().register(s -> {
			randomValueDisplayString.setText(s.randomizeAmountDisplayString);
			randomValueSliderRange.setValue(s.randomizeAmountValue);

			tuneReferenceDisplayString.setText(s.tuneReferenceDisplayString);
			tuneReferenceSliderRange.setValue(s.tuneReferenceValue);

			transitionTimeDisplayString.setText(s.transitionTimeDisplayString);
			transitionTimeSliderRange.setValue(s.transitionTimeValue);
			return true;
		});
	}

}
