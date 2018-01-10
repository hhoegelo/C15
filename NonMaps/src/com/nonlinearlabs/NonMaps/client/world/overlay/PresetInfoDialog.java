package com.nonlinearlabs.NonMaps.client.world.overlay;

import java.util.Date;

import com.google.gwt.event.dom.client.BlurEvent;
import com.google.gwt.event.dom.client.BlurHandler;
import com.google.gwt.event.dom.client.FocusEvent;
import com.google.gwt.event.dom.client.FocusHandler;
import com.google.gwt.event.dom.client.KeyCodes;
import com.google.gwt.event.dom.client.KeyPressEvent;
import com.google.gwt.event.dom.client.KeyPressHandler;
import com.google.gwt.i18n.client.DateTimeFormat;
import com.google.gwt.i18n.client.DateTimeFormat.PredefinedFormat;
import com.google.gwt.user.client.ui.FlexTable;
import com.google.gwt.user.client.ui.HTMLPanel;
import com.google.gwt.user.client.ui.IntegerBox;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.RootPanel;
import com.google.gwt.user.client.ui.TextArea;
import com.google.gwt.user.client.ui.TextBox;
import com.google.gwt.user.client.ui.Widget;
import com.nonlinearlabs.NonMaps.client.NonMaps;
import com.nonlinearlabs.NonMaps.client.world.maps.presets.bank.Bank;
import com.nonlinearlabs.NonMaps.client.world.maps.presets.bank.preset.Preset;

public class PresetInfoDialog extends GWTDialog {

	private static PresetInfoDialog theDialog;
	private TextArea comment;
	private Label deviceName;
	private Label softwareVersion;
	private Label storeTime;
	private Label bankName;
	private Widget haveFocus = null;
	private Preset thePreset;
	private Preset theEditPreset;
	private TextBox name;
	private IntegerBox position;

	private PresetInfoDialog() {
		RootPanel.get().add(this);

		getElement().addClassName("preset-info-dialog");

		initalShow();

		setAnimationEnabled(true);
		setGlassEnabled(false);
		setModal(false);

		addHeader("Preset Info");
		addContent();

		initialSetup();
	}

	private void initialSetup() {
		updateInfo(NonMaps.theMaps.getNonLinearWorld().getPresetManager().getSelectedPreset());
	}

	private void addRow(FlexTable panel, String name, Widget content) {
		int c = panel.getRowCount();
		panel.setWidget(c, 0, new Label(name));
		panel.setWidget(c, 1, content);
	}

	private void addContent() {
		HTMLPanel bankNameAndPosition = new HTMLPanel("div", "");
		bankNameAndPosition.getElement().addClassName("flex-div bankname-and-position");
		bankNameAndPosition.add(bankName = new Label());
		bankNameAndPosition.add(position = new IntegerBox());
		bankName.getElement().addClassName("gwt-TextBox");

		HTMLPanel presetNameBox = new HTMLPanel("div", "");
		presetNameBox.getElement().addClassName("flex-div name-box");
		presetNameBox.add(name = new TextBox());
		name.getElement().addClassName("name");

		FlexTable panel = new FlexTable();
		addRow(panel, "Name", presetNameBox);
		addRow(panel, "Position", bankNameAndPosition);
		addRow(panel, "Comment", comment = new TextArea());
		addRow(panel, "Device Name", deviceName = new Label(""));
		addRow(panel, "Software Version", softwareVersion = new Label(""));
		addRow(panel, "Store Time", storeTime = new Label(""));

		position.getElement().addClassName("gwt-TextBox");

		comment.addFocusHandler(new FocusHandler() {

			@Override
			public void onFocus(FocusEvent event) {
				haveFocus = comment;
				theEditPreset = thePreset;
			}
		});

		comment.addBlurHandler(new BlurHandler() {

			@Override
			public void onBlur(BlurEvent event) {
				haveFocus = null;

				if (theEditPreset != null) {
					String oldInfo = theEditPreset.getAttribute("Comment");

					if (!oldInfo.equals(comment.getText())) {
						NonMaps.theMaps.getServerProxy().setPresetAttribute(theEditPreset, "Comment", comment.getText());
					}
				}
			}
		});

		name.addFocusHandler(new FocusHandler() {

			@Override
			public void onFocus(FocusEvent event) {
				haveFocus = name;
				theEditPreset = thePreset;
			}
		});

		name.addBlurHandler(new BlurHandler() {

			@Override
			public void onBlur(BlurEvent event) {
				haveFocus = null;

				if (theEditPreset != null) {
					String oldName = theEditPreset.getCurrentName();

					if (!oldName.equals(name.getText())) {
						NonMaps.theMaps.getServerProxy().renamePreset(theEditPreset.getUUID(), name.getText());
					}
				}
			}
		});

		name.addKeyPressHandler(new KeyPressHandler() {

			@Override
			public void onKeyPress(KeyPressEvent arg0) {
				if (arg0.getCharCode() == KeyCodes.KEY_ENTER) {
					name.setFocus(false);
					position.setFocus(true);
				}
			}
		});

		position.addFocusHandler(new FocusHandler() {

			@Override
			public void onFocus(FocusEvent event) {
				haveFocus = position;
				theEditPreset = thePreset;
			}
		});

		position.addBlurHandler(new BlurHandler() {

			@Override
			public void onBlur(BlurEvent event) {
				haveFocus = null;

				if (theEditPreset != null) {
					int oldNumber = theEditPreset.getNumber();
					Integer newPos = position.getValue();
					if (newPos != null) {
						if (!newPos.equals(oldNumber)) {
							Bank bank = theEditPreset.getParent();
							int presetCount = bank.getPresetCount();
							int targetPos = newPos.intValue();
							targetPos = Math.max(targetPos, 1);
							targetPos = Math.min(targetPos, presetCount);

							if (targetPos == presetCount)
								NonMaps.theMaps.getServerProxy().movePresetBelow(theEditPreset, bank.getLast());
							else if (targetPos > oldNumber)
								NonMaps.theMaps.getServerProxy().movePresetBelow(theEditPreset, bank.getPreset(targetPos - 1));
							else
								NonMaps.theMaps.getServerProxy().movePresetAbove(theEditPreset, bank.getPreset(targetPos - 1));

							return;
						}
					}

					position.setValue(theEditPreset.getNumber());

				}
			}
		});

		position.addKeyPressHandler(new KeyPressHandler() {

			@Override
			public void onKeyPress(KeyPressEvent arg0) {
				if (arg0.getCharCode() == KeyCodes.KEY_ENTER) {
					position.setFocus(false);
					comment.setFocus(true);
				}
			}
		});

		setWidget(panel);
	}

	@Override
	protected void commit() {
		hide();
		theDialog = null;
		NonMaps.theMaps.captureFocus();
		NonMaps.theMaps.getNonLinearWorld().requestLayout();
	}

	public static void toggle() {
		if (theDialog != null) {
			theDialog.commit();
		} else {
			if (!NonMaps.theMaps.getNonLinearWorld().getPresetManager().isEmpty())
				theDialog = new PresetInfoDialog();
		}
	}

	public static boolean isShown() {
		return theDialog != null;
	}

	public static void update(Preset preset) {
		if (theDialog != null)
			theDialog.updateInfo(preset);
	}

	private void updateInfo(Preset preset) {
		thePreset = preset;

		if (preset != null) {
			String presetName = preset.getCurrentName();
			deviceName.setText(preset.getAttribute("DeviceName"));
			softwareVersion.setText(preset.getAttribute("SoftwareVersion"));
			storeTime.setText(localizeTime(preset.getAttribute("StoreTime")));
			String commentText = preset.getAttribute("Comment");

			if (haveFocus != comment) {
				if (!commentText.equals(comment.getText())) {
					comment.setText(commentText);
				}
			}

			if (haveFocus != name) {
				if (!presetName.equals(name.getText())) {
					name.setText(presetName);
				}
			}

			if (haveFocus != position) {
				position.setValue(preset.getNumber());
			}

			Bank bank = preset.getParent();
			bankName.setText(bank.getOrderNumber() + ": " + bank.getTitleName());

			centerIfOutOfView();
		}
	}

	private String localizeTime(String iso) {
		try {
			DateTimeFormat f = DateTimeFormat.getFormat("yyyy-MM-ddTHH:mm:ssZZZZ");
			Date d = f.parse(iso);
			DateTimeFormat locale = DateTimeFormat.getFormat(PredefinedFormat.DATE_TIME_SHORT);
			return locale.format(d);
		} catch (Exception e) {
			return iso;
		}
	}

	static int lastPopupLeft = -1;
	static int lastPopupTop = -1;

	@Override
	protected void setLastPopupPos(int popupLeft, int popupTop) {
		lastPopupLeft = popupLeft;
		lastPopupTop = popupTop;
	}

	@Override
	protected int getLastPopupPosTop() {
		return lastPopupTop;
	}

	@Override
	protected int getLastPopupPosLeft() {
		return lastPopupLeft;
	}

	public static void update() {
		update(NonMaps.theMaps.getNonLinearWorld().getPresetManager().getSelectedPreset());
	}
}