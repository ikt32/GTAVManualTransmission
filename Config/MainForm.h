#pragma once
#include "../Gears/Util/Logger.hpp"

namespace GearsUI {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for MainForm
	/// </summary>
	public ref class MainForm : public System::Windows::Forms::Form
	{
	public:
		MainForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
			logger = new Logger("./Config.log");
			logger->Clear();
			logger->Write("Manual Transmission v4.2.0 Configuration Tool");
		}

	private:
		Logger *logger;

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~MainForm()
		{
			if (components)
			{
				delete components;
			}
			delete logger;
		}
	private: System::Windows::Forms::TabPage^  tabWheel;
	private: System::Windows::Forms::TabPage^  tabMain;
	private: System::Windows::Forms::TabControl^  tabControl;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->tabWheel = (gcnew System::Windows::Forms::TabPage());
			this->tabMain = (gcnew System::Windows::Forms::TabPage());
			this->tabControl = (gcnew System::Windows::Forms::TabControl());
			this->tabControl->SuspendLayout();
			this->SuspendLayout();
			// 
			// tabWheel
			// 
			this->tabWheel->Location = System::Drawing::Point(4, 22);
			this->tabWheel->Name = L"tabWheel";
			this->tabWheel->Padding = System::Windows::Forms::Padding(3);
			this->tabWheel->Size = System::Drawing::Size(912, 511);
			this->tabWheel->TabIndex = 1;
			this->tabWheel->Text = L"Wheel Configuration";
			this->tabWheel->UseVisualStyleBackColor = true;
			// 
			// tabMain
			// 
			this->tabMain->Location = System::Drawing::Point(4, 22);
			this->tabMain->Name = L"tabMain";
			this->tabMain->Padding = System::Windows::Forms::Padding(3);
			this->tabMain->Size = System::Drawing::Size(912, 511);
			this->tabMain->TabIndex = 0;
			this->tabMain->Text = L"Main Configuration";
			this->tabMain->UseVisualStyleBackColor = true;
			// 
			// tabControl
			// 
			this->tabControl->Controls->Add(this->tabMain);
			this->tabControl->Controls->Add(this->tabWheel);
			this->tabControl->Location = System::Drawing::Point(12, 12);
			this->tabControl->Name = L"tabControl";
			this->tabControl->SelectedIndex = 0;
			this->tabControl->Size = System::Drawing::Size(920, 537);
			this->tabControl->TabIndex = 1;
			// 
			// MainForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(944, 561);
			this->Controls->Add(this->tabControl);
			this->Name = L"MainForm";
			this->Text = L"Manual Transmission Configuration";
			this->tabControl->ResumeLayout(false);
			this->ResumeLayout(false);

		}
#pragma endregion

	};
}
