package com.nonlinearlabs.NonMaps.client.dataModel.editBuffer;

import com.google.gwt.xml.client.Node;
import com.nonlinearlabs.NonMaps.client.dataModel.Updater;

public class ParameterGroupUpdater extends Updater {
	private ParameterGroupModel target;

	public ParameterGroupUpdater(Node c, ParameterGroupModel target) {
		super(c);
		this.target = target;
	}

	@Override
	public void doUpdate() {
		target.shortName.setValue(getAttributeValue(root, "short-name"));
		target.longName.setValue(getAttributeValue(root, "long-name"));
		processChangedChildrenElements(root, "parameter", c -> processParameter(c));
	}

	private void processParameter(Node c) {
		String id = getAttributeValue(c, "id");
		BasicParameterModel p = EditBufferModel.get().findParameter(Integer.parseInt(id));
		if (p != null) {
			Updater u = p.getUpdater(c);
			u.doUpdate();
		}
	}
}
