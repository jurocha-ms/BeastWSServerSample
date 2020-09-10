#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Networking.Sockets.h>
#include <winrt/Windows.Security.Cryptography.Certificates.h>
#include <winrt/Windows.Storage.Streams.h>

#include <Windows.h>

#include <chrono>
#include <iostream>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Networking::Sockets;
using namespace winrt::Windows::Security::Cryptography::Certificates;
using namespace winrt::Windows::Storage::Streams;

using std::cerr;
using std::cout;
using std::endl;
using std::wcerr;
using std::wcout;

auto s_webSocket = MessageWebSocket {};
winrt::handle s_readEvent { ::CreateEvent(nullptr, true, false, nullptr) };

void OnMessageReceived(MessageWebSocket const& sender, MessageWebSocketMessageReceivedEventArgs const& args)
{
	auto reader { args.GetDataReader() };
	reader.UnicodeEncoding(UnicodeEncoding::Utf8);

	auto message = reader.ReadString(reader.UnconsumedBufferLength());
	wcout << L"[SERVER] " <<  message.c_str() << endl;

	::SetEvent(s_readEvent.get());
	s_readEvent.close();
}

void OnClosed(IWebSocket const&, WebSocketClosedEventArgs const& args)
{
	wcout << L"[CLOSED] Code: " << args.Code() << L" Reason: " << args.Reason().c_str() << endl;
}

IAsyncAction SendMessageAsync(std::wstring message)
{
	DataWriter writer { s_webSocket.OutputStream() };
	writer.WriteString(message);

	co_await writer.StoreAsync();
	writer.DetachStream();
}

IAsyncAction MainAsync()
{
	s_webSocket.Control().MessageType(SocketMessageType::Utf8);
	s_webSocket.Control().IgnorableServerCertificateErrors().Append(ChainValidationResult::Untrusted);
	s_webSocket.Control().IgnorableServerCertificateErrors().Append(ChainValidationResult::InvalidName);

	auto messageReceivedToken = s_webSocket.MessageReceived({ &OnMessageReceived });
	auto closedEventToken = s_webSocket.Closed({ &OnClosed });

	try
	{
		co_await s_webSocket.ConnectAsync(Uri { L"wss://localhost:5556" });
		//co_await s_webSocket.ConnectAsync(Uri{L"wss://echo.websocket.org"});
		cout << "[CONNECTED] " << endl;

		co_await SendMessageAsync(L"HELLO!");
		cout << "[SENT]" << endl;

		co_await winrt::resume_on_signal(s_readEvent.get()/*, std::chrono::seconds(5)*/);

		s_webSocket.Close();
	}
	catch (winrt::hresult_error& e)
	{
		wcerr << L"[ERROR] " << e.message().c_str() << endl;
	}

	cout << "[FINISH]" << endl;
}

int main()
{
	winrt::init_apartment();

	auto mainAction { MainAsync() };

	mainAction.get();
}