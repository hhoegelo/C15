package com.nonlinearlabs.client.presenters;

import java.util.LinkedList;
import java.util.function.Function;

public class DeviceInformationProvider {
	public static DeviceInformationProvider theInstance = new DeviceInformationProvider();

	public static DeviceInformationProvider get() {
		return theInstance;
	}

	private LinkedList<Function<DeviceInformation, Boolean>> clients = new LinkedList<Function<DeviceInformation, Boolean>>();
	private DeviceInformation info = new DeviceInformation();

	private DeviceInformationProvider() {
		com.nonlinearlabs.client.dataModel.setup.DeviceInformation.get().freeDiscSpace.onChange(v -> {
			info.freeDiscSpace = v;
			notifyClients();
			return true;
		});

		com.nonlinearlabs.client.dataModel.setup.DeviceInformation.get().softwareVersion.onChange(v -> {
			info.uiVersion = v;
			notifyClients();
			return true;
		});

		com.nonlinearlabs.client.dataModel.setup.DeviceInformation.get().rtSoftwareVersion.onChange(v -> {
			info.rtVersion = v;
			notifyClients();
			return true;
		});

		com.nonlinearlabs.client.dataModel.setup.DeviceInformation.get().osVersion.onChange(v -> {
			info.osVersion = v;
			notifyClients();
			return true;
		});

		com.nonlinearlabs.client.dataModel.setup.DeviceInformation.get().dateTimeDisplay.onChange(v -> {
			info.dateTime = v;
			notifyClients();
			return true;
		});
	}

	protected void notifyClients() {
		clients.removeIf(listener -> !listener.apply(info));
	}

	public void register(Function<DeviceInformation, Boolean> cb) {
		clients.add(cb);
		cb.apply(info);
	}
}
