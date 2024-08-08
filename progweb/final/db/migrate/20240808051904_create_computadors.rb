class CreateComputadors < ActiveRecord::Migration[7.1]
  def change
    create_table :computadors do |t|
      t.string :numserie
      t.string :marca
      t.references :pessoa, null: false, foreign_key: true

      t.timestamps
    end
  end
end
